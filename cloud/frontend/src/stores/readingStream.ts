/**
 * Story 6.3 + 6.4 + 6.5：阅读流 Pinia store（非权威展示态，AD-2 / AD-17 / AD-19）。
 *
 * 裁决（6.3 保留 + 6.4 扩展 + 6.5 完成态）：
 * - A（6.3）：字形 = round_cursor + STEPS；禁止 glyph wire。
 * - B（6.3）：动画队列逐步 ++；禁止一次跳到末尾。
 * - C（6.4 覆盖）：若持久化 replay_cursor>0 且落后于快照 round_cursor，强制 animate 追赶；
 *   无持久化或 replay_cursor===0 允许一次性水合（避免首登超长回放）。
 * - A（6.4）：replay_cursor = displayedCursor 持久化镜像；正式进度仍信快照/delta。
 * - B（6.4）：REPLAY 期间 pendingDeltas 排队；追上后按 seq 升序 flush。
 * - D（6.4）：可恢复断开 → offline + 退避；1008/契约拒绝 → error 不重连。
 * - E（6.4）：水位步进后 persist；登出/reset 清三键。
 * - F（6.4）：banner 文案对拍 HTML；本 store 只暴露 uiPhase。
 * - G（6.4）：暴露 isReplaying；6.5 不在 OFFLINE 误弹完成窗。
 * - A（6.5）：uiPhase='done' + overlayVisible；门控 round_state==='completed' && !pendingCompletion
 *   && !isReplaying && displayedCursor >= roundCursor。
 * - B（6.5）：overlay 打开时 delta 不推进正文；round_id 变化关窗 resync；celebrationPlayedForRoundId 防重复礼花。
 * - C（6.5）：action_id 同意图复用至成功；请求期 actionBusy 单飞。
 * - D（6.5）：restart 将当前全文推入 archives[]（展示态，非 AD-2 权威）；默认折叠。
 * - E（6.5）：exit 关 overlay，保持 done + 全文/100%。
 * - F（6.5）：礼花由 UI 短促播放；store 只发 celebrationPending。
 * - G（6.5）：DONE banner 优先于 LIVE；不改 REPLAY 排队核心。
 *
 * 权威基准永远是快照 snapshot_seq，不是 displayedCursor。
 * 完成只信 backend（pending_completion 禁止抢跑）。
 */
import { defineStore } from 'pinia'
import { getSyncSnapshot, postRoundAction } from '../api/sync'
import { SUCCESS_CODE } from '../utils/constants'
import { enqueueSteps, stepIntervalMs } from '../utils/animationSchedule'
import {
  nextBackoffMs,
  RECONNECT_POLICY,
  shouldReconnectOnClose,
} from '../utils/reconnectPolicy'
import {
  closeRealtimeSocket,
  isRealtimeSocketOpen,
  openRealtimeSocket,
} from '../utils/realtimeSocket'
import { displayCharsUpTo } from '../utils/scriptureProjection'
import {
  clearReadingWatermarks,
  getAccessToken,
  getPersistedLastAppliedSeq,
  getPersistedReplayCursor,
  getPersistedSnapshotSeq,
  persistReadingWatermarks,
} from '../utils/storage'
import type {
  RoundActionKind,
  StateSnapshot,
  WsDeltaFrame,
  WsInboundFrame,
} from '../types/sync'

export type ReadingUiPhase =
  | 'empty'
  | 'live'
  | 'replay'
  | 'offline'
  | 'connecting'
  | 'error'
  | 'done'

/** 篇章归档（UX-DR18 展示行为；非 cloud 权威；上限见 deferred）。 */
export interface ArchiveChapter {
  roundId: number
  chars: string[]
  collapsed: boolean
}

type TimerHandle = ReturnType<typeof setTimeout> | null

/** 裁决 C：不透明 action_id；与设置命令族键空间隔离。 */
export function createRoundActionId(action: RoundActionKind): string {
  const rand = Math.random().toString(36).slice(2, 10)
  return `ewf_ra_${action}_${Date.now()}_${rand}`
}

export const useReadingStreamStore = defineStore('readingStream', {
  state: () => ({
    /** 权威镜像（backend）；不作本地假累计。 */
    roundId: 0,
    roundCursor: 0,
    ackedTotal: 0,
    snapshotSeq: 0,
    localTotal: 0,
    roundState: 'in_progress' as string,
    /** Story 6.5：镜像 pending_completion；true 时禁止弹窗/本轮完成文案。 */
    pendingCompletion: false,
    /** 展示游标：动画追上权威前可滞后；持久化镜像 = replay_cursor。 */
    displayedCursor: 0,
    /** 已应用的最大 seq（= snapshot_seq 或 delta.seq）。 */
    lastAppliedSeq: 0,
    uiPhase: 'empty' as ReadingUiPhase,
    liveActive: false,
    lastError: '' as string,
    /** 需要重拉快照（空洞 / 轮次错位 / error 帧）。 */
    needsResync: false,
    /** 裁决 B：REPLAY 期间新 delta 排队，禁止插队。 */
    pendingDeltas: [] as WsDeltaFrame[],
    /** 本次回放要追上的冻结目标（快照 round_cursor）。 */
    replayTargetCursor: 0,
    reconnectAttempts: 0,
    /** Story 6.5：完成弹窗可见。 */
    overlayVisible: false,
    /** 弹窗期冻结展示游标（权威仍可更新）。 */
    displayFrozen: false,
    /** 已为该 round_id 播过礼花/开过弹窗。 */
    celebrationPlayedForRoundId: 0,
    /** exit 后本轮不再自动开窗（与礼花门闩分离）。 */
    overlayDismissedForRoundId: 0,
    /** UI 礼花一次触发；播完后 clearCelebration。 */
    celebrationPending: false,
    /** 已完成篇章归档（内存展示；登出清）。 */
    archives: [] as ArchiveChapter[],
    /** 裁决 C：进行中的 action_id（失败可同键重试）。 */
    inflightActionId: null as string | null,
    inflightAction: null as RoundActionKind | null,
    actionBusy: false,
    _timer: null as TimerHandle,
    _reconnectTimer: null as TimerHandle,
    _hydratedOnce: false,
    _flushingPending: false,
  }),
  getters: {
    /** 动画积压深度。 */
    backlog(state): number {
      return Math.max(0, state.roundCursor - state.displayedCursor)
    },
    isEmpty(state): boolean {
      return state.displayedCursor === 0 && state.roundCursor === 0 && !state._timer
        && state.uiPhase !== 'done'
    },
    /** 裁决 G：6.5 可据此避免在 REPLAY/OFFLINE 误弹完成窗。 */
    isReplaying(state): boolean {
      return state.uiPhase === 'replay' || state.uiPhase === 'offline'
    },
  },
  actions: {
    loadPersistedWatermarks(): void {
      const snap = getPersistedSnapshotSeq()
      const last = getPersistedLastAppliedSeq()
      const replay = getPersistedReplayCursor()
      if (snap != null) {
        this.snapshotSeq = snap
      }
      if (last != null) {
        this.lastAppliedSeq = last
      }
      if (replay != null) {
        this.displayedCursor = replay
      }
    },

    persistWatermarks(): void {
      persistReadingWatermarks({
        snapshotSeq: this.snapshotSeq,
        replayCursor: this.displayedCursor,
        lastAppliedSeq: this.lastAppliedSeq,
      })
    },

    /**
     * 裁决 C：快照水合。
     * - animate=false：一次性对齐展示（冷启动无持久化回放）。
     * - animate=true 且展示落后：进入 REPLAY 逐字追赶。
     */
    applySnapshot(snapshot: StateSnapshot, options: { animate?: boolean } = {}): void {
      const animate = options.animate === true
      const prevRound = this.roundId
      const nextRound = snapshot.round_id
      const nextCursor = Math.max(0, snapshot.round_cursor)

      this.ackedTotal = snapshot.acked_total
      this.localTotal = snapshot.local_total
      this.snapshotSeq = snapshot.snapshot_seq
      this.roundState = snapshot.round_state
      this.pendingCompletion = snapshot.pending_completion === true
      // 权威基准 = 查询冻结的 snapshot_seq（禁止用展示计数推断）
      this.lastAppliedSeq = snapshot.snapshot_seq
      this.needsResync = false
      this.lastError = ''

      if (prevRound !== 0 && nextRound !== prevRound) {
        // 裁决 B：弹窗期若 round 变化，关窗后走新轮水合
        this.closeOverlayKeepPhase()
        this.overlayDismissedForRoundId = 0
        this.celebrationPlayedForRoundId = 0
        this.stopAnimationTimer()
        this.pendingDeltas = []
        this.roundId = nextRound
        this.roundCursor = nextCursor
        this.displayedCursor = animate ? 0 : nextCursor
        this.replayTargetCursor = animate ? nextCursor : 0
        if (animate && nextCursor > 0) {
          this.uiPhase = 'replay'
          this.ensureAnimationTimer()
        }
      } else {
        this.roundId = nextRound
        this.roundCursor = nextCursor
        if (!animate || !this._hydratedOnce) {
          this.stopAnimationTimer()
          this.displayedCursor = nextCursor
          this.replayTargetCursor = 0
        } else if (nextCursor > this.displayedCursor && !this.displayFrozen) {
          this.replayTargetCursor = nextCursor
          this.uiPhase = 'replay'
          this.ensureAnimationTimer()
        } else if (nextCursor < this.displayedCursor) {
          this.displayedCursor = nextCursor
          this.replayTargetCursor = 0
        } else {
          this.replayTargetCursor = 0
        }
      }

      this._hydratedOnce = true
      this.persistWatermarks()
      // pending_completion 抢跑：已开窗则关窗，并离开 done 声称
      if (this.pendingCompletion) {
        if (this.overlayVisible) {
          this.closeOverlayKeepPhase()
        }
      }
      if (this.uiPhase !== 'replay') {
        this.refreshPhase()
      }
      this.enterDoneIfEligible()
    },

    /**
     * LIVE delta：校验后推进目标游标；中间步进入队逐字展示。
     * 无 glyph 字段依赖。
     * 裁决 B（6.5）：overlay/冻结时只更新权威镜像，不推进正文。
     */
    applyDelta(frame: WsDeltaFrame): 'applied' | 'stale' | 'hole' | 'round_mismatch' {
      if (frame.seq <= this.lastAppliedSeq) {
        return 'stale'
      }
      // 含 lastAppliedSeq=0：seq 相对水位跳号即空洞，禁止静默吞中间步进
      if (frame.seq > this.lastAppliedSeq + 1) {
        this.needsResync = true
        return 'hole'
      }
      if (this.roundId !== 0 && frame.round_id !== this.roundId) {
        // 裁决 B：弹窗期设备侧已 restart → 关窗并 resync
        if (this.overlayVisible || this.displayFrozen) {
          this.closeOverlayKeepPhase()
        }
        this.needsResync = true
        return 'round_mismatch'
      }

      this.roundId = frame.round_id
      this.ackedTotal = frame.acked_total
      this.localTotal = frame.local_total
      this.roundCursor = Math.max(0, frame.round_cursor)
      this.lastAppliedSeq = frame.seq
      this.persistWatermarks()

      if (this.overlayVisible || this.displayFrozen) {
        // 展示游标冻结于完成全文；不 append
        return 'applied'
      }

      this.ensureAnimationTimer()
      if (this.uiPhase !== 'replay' && this.uiPhase !== 'offline') {
        this.refreshPhase()
      }
      return 'applied'
    },

    /** 裁决 B：REPLAY 时入队，禁止插入当前回放差量中间。 */
    enqueuePendingDelta(frame: WsDeltaFrame): void {
      if (this.pendingDeltas.some((d) => d.seq === frame.seq)) {
        return
      }
      this.pendingDeltas.push(frame)
    },

    /**
     * 展示追上 replayTargetCursor 后按 seq 升序消费排队帧。
     * flush 中 hole/round_mismatch → 中断并以快照为准重建。
     */
    flushPendingAfterReplay(): void {
      if (this._flushingPending) {
        return
      }
      this._flushingPending = true
      try {
        this.pendingDeltas.sort((a, b) => a.seq - b.seq)
        while (this.pendingDeltas.length > 0) {
          const next = this.pendingDeltas[0]!
          const result = this.applyDelta(next)
          if (result === 'stale') {
            this.pendingDeltas.shift()
            continue
          }
          if (result === 'hole' || result === 'round_mismatch') {
            this.pendingDeltas = []
            this.replayTargetCursor = 0
            // 避免空 flush 在异步 resync 完成前把相位误标为 live
            this.uiPhase = 'connecting'
            void this.resyncFromSnapshot({ animate: true })
            return
          }
          this.pendingDeltas.shift()
        }
        this.replayTargetCursor = 0
        if (this.displayedCursor === 0 && this.roundCursor === 0) {
          this.uiPhase = 'empty'
        } else {
          this.uiPhase = 'live'
        }
        this.enterDoneIfEligible()
      } finally {
        this._flushingPending = false
      }
    },

    maybeCompleteReplay(): void {
      if (this.uiPhase !== 'replay') {
        return
      }
      if (this.displayedCursor < this.replayTargetCursor) {
        return
      }
      this.flushPendingAfterReplay()
    },

    /**
     * 裁决 A：完成门控。快照与 completion 帧共用。
     * pendingCompletion===true 只更新镜像，不弹窗。
     * exit 用 overlayDismissedForRoundId 抑制重开；礼花门闩只抑重复粒子。
     */
    enterDoneIfEligible(): void {
      if (this.roundState !== 'completed' || this.pendingCompletion) {
        return
      }
      if (this.isReplaying) {
        return
      }
      if (this.uiPhase === 'error' || this.uiPhase === 'connecting') {
        return
      }
      // 全文已展示后再弹，避免半屏礼花
      if (this.displayedCursor < this.roundCursor) {
        return
      }

      this.uiPhase = 'done'
      if (this.roundId === 0) {
        return
      }
      // 用户 exit：本轮保持 done 全文，不再开窗
      if (this.overlayDismissedForRoundId === this.roundId) {
        this.displayFrozen = true
        return
      }
      const alreadyCelebrated = this.celebrationPlayedForRoundId === this.roundId
      // mismatch 关窗后同轮仍 completed：可重开窗，但不重复礼花
      if (!this.overlayVisible) {
        this.overlayVisible = true
        this.displayFrozen = true
      } else {
        this.displayFrozen = true
      }
      if (!alreadyCelebrated) {
        this.celebrationPlayedForRoundId = this.roundId
        this.celebrationPending = true
      }
    },

    /** 关弹窗；keepFrozen 用于 exit 后继续锁住完成全文。 */
    closeOverlayKeepPhase(options: { keepFrozen?: boolean } = {}): void {
      this.overlayVisible = false
      this.celebrationPending = false
      if (!options.keepFrozen) {
        this.displayFrozen = false
      }
    },

    clearCelebration(): void {
      this.celebrationPending = false
    },

    toggleArchive(roundId: number): void {
      const item = this.archives.find((a) => a.roundId === roundId)
      if (item) {
        item.collapsed = !item.collapsed
      }
    },

    /**
     * 裁决 C/D：从头开始。单飞锁；成功后归档当前篇并以响应快照水合。
     */
    async restartRound(): Promise<boolean> {
      if (this.actionBusy) {
        return false
      }
      if (this.inflightAction !== 'restart' || !this.inflightActionId) {
        this.inflightAction = 'restart'
        this.inflightActionId = createRoundActionId('restart')
      }
      const actionId = this.inflightActionId
      this.actionBusy = true
      try {
        const body = await postRoundAction({ action: 'restart', action_id: actionId })
        if (body.code !== SUCCESS_CODE || !body.data) {
          this.lastError = body.message || '跨轮动作失败'
          return false
        }
        // 裁决 D：归档当前全文（展示态，非权威）；默认折叠；同 roundId 不重复入档
        const archiveChars = displayCharsUpTo(this.displayedCursor)
        if (
          this.roundId > 0
          && archiveChars.length > 0
          && !this.archives.some((a) => a.roundId === this.roundId)
        ) {
          this.archives.unshift({
            roundId: this.roundId,
            chars: archiveChars,
            collapsed: true,
          })
        }
        this.overlayDismissedForRoundId = 0
        this.closeOverlayKeepPhase()
        this.celebrationPlayedForRoundId = 0
        this.inflightActionId = null
        this.inflightAction = null
        this.applySnapshot(body.data, { animate: false })
        return true
      } catch (err) {
        this.lastError = err instanceof Error ? err.message : '跨轮动作失败'
        return false
      } finally {
        this.actionBusy = false
      }
    },

    /**
     * 裁决 C/E：退出。关 overlay，保留 completed 全文与 100%。
     */
    async exitRound(): Promise<boolean> {
      if (this.actionBusy) {
        return false
      }
      if (this.inflightAction !== 'exit' || !this.inflightActionId) {
        this.inflightAction = 'exit'
        this.inflightActionId = createRoundActionId('exit')
      }
      const actionId = this.inflightActionId
      this.actionBusy = true
      try {
        const body = await postRoundAction({ action: 'exit', action_id: actionId })
        if (body.code !== SUCCESS_CODE || !body.data) {
          this.lastError = body.message || '跨轮动作失败'
          return false
        }
        this.overlayDismissedForRoundId = this.roundId
        this.closeOverlayKeepPhase({ keepFrozen: true })
        this.inflightActionId = null
        this.inflightAction = null
        this.applySnapshot(body.data, { animate: false })
        // exit 后权威仍 completed → 保持 done，不再开窗
        if (this.roundState === 'completed' && !this.pendingCompletion) {
          this.uiPhase = 'done'
          this.displayFrozen = true
        }
        return true
      } catch (err) {
        this.lastError = err instanceof Error ? err.message : '跨轮动作失败'
        return false
      } finally {
        this.actionBusy = false
      }
    },

    handleWsFrame(frame: WsInboundFrame): void {
      if (frame.type === 'delta') {
        const delta = frame as WsDeltaFrame
        // 裁决 B：回放中新事件排队
        if (this.uiPhase === 'replay') {
          this.enqueuePendingDelta(delta)
          return
        }
        const result = this.applyDelta(delta)
        if (result === 'hole' || result === 'round_mismatch') {
          void this.resyncFromSnapshot({ animate: true })
        }
        return
      }
      if (frame.type === 'completion') {
        // 镜像完成权威；pending_completion=true 不弹窗
        this.roundState = frame.round_state
        this.pendingCompletion = frame.pending_completion === true
        if (this.pendingCompletion && this.overlayVisible) {
          this.closeOverlayKeepPhase()
        }
        // REPLAY 中禁止抬 lastAppliedSeq，否则排队 delta 会被当成 stale
        if (this.uiPhase === 'replay') {
          return
        }
        if (typeof frame.seq === 'number' && frame.seq > this.lastAppliedSeq) {
          this.lastAppliedSeq = frame.seq
          this.persistWatermarks()
        }
        this.refreshPhase()
        this.enterDoneIfEligible()
        return
      }
      if (frame.type === 'error') {
        this.lastError = (frame as { error?: { message?: string } }).error?.message || '同步错误'
        this.uiPhase = 'error'
        closeRealtimeSocket()
        void this.resyncFromSnapshot({ animate: true })
      }
      // snapshot / command_state：本 Story 忽略（水位靠 REST）
    },

    async startLiveSession(): Promise<void> {
      // Task 4.3：liveActive 已 true 时仍做一次幂等水位对齐，不双开 socket
      if (this.liveActive) {
        await this.resyncFromSnapshot({ animate: true, alignOnly: true })
        return
      }
      this.liveActive = true
      // onHide/onShow：保留 DONE，避免闪成 connecting 冲掉完成文案
      if (this.uiPhase !== 'done') {
        this.uiPhase = 'connecting'
      }
      this.reconnectAttempts = 0
      this.loadPersistedWatermarks()
      await this.resyncFromSnapshot({ animate: true })
      // 快照失败不得盲连 WS（否则 lastAppliedSeq=0 时差量可错序入账）
      if (this.uiPhase === 'error' || this.uiPhase === 'offline' || !this._hydratedOnce) {
        return
      }
      // resync 成功路径可能已建连；禁止无条件二次 openRealtimeSocket（会先关再建）
      if (!isRealtimeSocketOpen()) {
        this.connectSocket()
      }
    },

    stopLiveSession(): void {
      this.liveActive = false
      this.clearReconnectTimer()
      this.stopAnimationTimer()
      closeRealtimeSocket()
      this.pendingDeltas = []
      this.replayTargetCursor = 0
      if (this.uiPhase !== 'error' && this.uiPhase !== 'done') {
        this.refreshPhase()
      }
    },

    /**
     * 用水位冻结补齐差量基准。权威 = 快照 snapshot_seq。
     * 裁决 C：有持久化展示缺口 → REPLAY；无缺口 → live/empty。
     */
    async resyncFromSnapshot(options: {
      animate?: boolean
      alignOnly?: boolean
    } = {}): Promise<void> {
      let body: Awaited<ReturnType<typeof getSyncSnapshot>>
      try {
        body = await getSyncSnapshot({
          acked_total: this.ackedTotal || undefined,
          snapshot_seq: this.snapshotSeq || undefined,
        })
      } catch (err) {
        this.lastError = err instanceof Error ? err.message : '快照不可用'
        if (this.uiPhase === 'done') {
          // DONE 断线补齐失败：保相位，仅退避重连（与 onClose 一致）
          this.scheduleReconnect()
          return
        }
        if (this.liveActive && this.reconnectAttempts < RECONNECT_POLICY.maxAttempts) {
          this.uiPhase = 'offline'
          this.scheduleReconnect()
        } else {
          this.uiPhase = 'error'
        }
        return
      }
      if (body.code === SUCCESS_CODE && body.data) {
        const snap = body.data
        const persistedReplay = getPersistedReplayCursor()
        const displayBase = persistedReplay != null
          ? persistedReplay
          : this.displayedCursor
        const nextCursor = Math.max(0, snap.round_cursor)

        // 裁决 C：用户曾展示过进度（replay_cursor>0）且权威超前 → 强制可感知回放
        const mustReplay = displayBase > 0 && displayBase < nextCursor

        if (mustReplay) {
          // 先恢复展示起点，再应用快照权威并动画追赶
          this.displayedCursor = displayBase
          this._hydratedOnce = true
          this.applySnapshot(snap, { animate: true })
        } else {
          // 无展示缺口：冷启动/已对齐允许硬切（裁决 C）；animate 选项不强制无缺口回放
          this.applySnapshot(snap, { animate: false })
        }

        this.needsResync = false
        this.reconnectAttempts = 0
        this.clearReconnectTimer()

        // alignOnly 只禁止「socket 已开时再建」；socket 已死仍须重连以便 REPLAY 排队
        if (this.liveActive && !isRealtimeSocketOpen()) {
          this.connectSocket()
        } else if (isRealtimeSocketOpen() && this.uiPhase !== 'replay') {
          this.refreshPhase()
        }
        void options.alignOnly
      } else {
        this.lastError = body.message || '快照不可用'
        if (this.uiPhase === 'done') {
          this.scheduleReconnect()
          return
        }
        // 裁决 D：可恢复失败先 offline 退避；耗尽 → error
        if (this.liveActive && this.reconnectAttempts < RECONNECT_POLICY.maxAttempts) {
          this.uiPhase = 'offline'
          this.scheduleReconnect()
        } else {
          this.uiPhase = 'error'
        }
      }
    },

    clearReconnectTimer(): void {
      if (this._reconnectTimer != null) {
        clearTimeout(this._reconnectTimer)
        this._reconnectTimer = null
      }
    },

    scheduleReconnect(): void {
      if (!this.liveActive) {
        return
      }
      // onError + onClose 可能连发：已调度则不重复占 attempt
      if (this._reconnectTimer != null) {
        return
      }
      if (this.reconnectAttempts >= RECONNECT_POLICY.maxAttempts) {
        this.uiPhase = 'error'
        this.lastError = this.lastError || '重连次数已用尽'
        return
      }
      const attempt = this.reconnectAttempts
      const delay = nextBackoffMs(attempt)
      this.reconnectAttempts = attempt + 1
      this._reconnectTimer = setTimeout(() => {
        this._reconnectTimer = null
        void this.recoverFromOffline()
      }, delay)
    },

    /** 断线恢复：先关旧 socket → 快照 → 有缺口 replay / 无缺口 live。 */
    async recoverFromOffline(): Promise<void> {
      if (!this.liveActive) {
        return
      }
      closeRealtimeSocket()
      await this.resyncFromSnapshot({ animate: true })
      if (this.uiPhase === 'error') {
        return
      }
      if (this.uiPhase === 'offline') {
        // resync 内部已再调度或置 error
        return
      }
      if (this.liveActive && !isRealtimeSocketOpen()) {
        this.connectSocket()
      }
    },

    connectSocket(): void {
      const token = getAccessToken()
      if (!token || !this.liveActive) {
        return
      }
      openRealtimeSocket(token, {
        onFrame: (frame) => this.handleWsFrame(frame),
        onOpen: () => {
          this.reconnectAttempts = 0
          if (this.uiPhase === 'offline' || this.uiPhase === 'connecting') {
            if (this.displayedCursor === 0 && this.roundCursor === 0) {
              this.uiPhase = 'empty'
            } else if (this.uiPhase !== 'replay') {
              this.uiPhase = 'live'
            }
            this.enterDoneIfEligible()
          } else if (this.uiPhase !== 'replay' && this.uiPhase !== 'error' && this.uiPhase !== 'done') {
            this.refreshPhase()
          }
        },
        onClose: (info) => {
          const code = info.code
          if (!this.liveActive) {
            return
          }
          // 裁决 D：1000/1008 不重连；1008 → error
          if (!shouldReconnectOnClose(code)) {
            this.stopAnimationTimer()
            if (code === 1008) {
              this.uiPhase = 'error'
              this.lastError = info.reason || '连接被拒绝'
            } else if (code === 1000) {
              // 会话仍活却收到正常关闭：禁止停在假 live（契约也不重连）
              this.uiPhase = 'error'
              this.lastError = info.reason || '连接已关闭'
            }
            return
          }
          this.stopAnimationTimer()
          // REPLAY 期间断线：保留回放相位，仍调度快照恢复；禁止冲掉 pending 消费路径
          if (this.uiPhase !== 'replay' && this.uiPhase !== 'done') {
            this.uiPhase = 'offline'
          } else if (this.uiPhase === 'done') {
            // done 态断线：不误弹新窗；标记 offline 会挡住 isReplaying——保持 done，仅调度恢复
            this.scheduleReconnect()
            return
          }
          this.scheduleReconnect()
        },
        onError: (message) => {
          this.lastError = message
          // AD-4 / 契约版本不一致：停止推进，不自动重连吞错字
          if (message.includes('contract_version')) {
            this.uiPhase = 'error'
            closeRealtimeSocket()
            return
          }
          if (!this.liveActive || this.uiPhase === 'error') {
            return
          }
          // socket 仍存活时（如单帧 JSON 解析失败）只记错，等 onClose 统一退避，避免双计 attempt
          if (isRealtimeSocketOpen()) {
            return
          }
          if (this.uiPhase !== 'replay' && this.uiPhase !== 'done') {
            this.uiPhase = 'offline'
          }
          this.scheduleReconnect()
        },
      })
    },

    ensureAnimationTimer(): void {
      if (this._timer != null) {
        return
      }
      if (this.displayFrozen) {
        return
      }
      if (this.displayedCursor >= this.roundCursor) {
        this.maybeCompleteReplay()
        this.enterDoneIfEligible()
        return
      }
      const tick = () => {
        this._timer = null
        if (this.displayFrozen) {
          return
        }
        if (this.displayedCursor >= this.roundCursor) {
          this.persistWatermarks()
          this.maybeCompleteReplay()
          if (this.uiPhase !== 'replay' && this.uiPhase !== 'offline') {
            this.refreshPhase()
          }
          this.enterDoneIfEligible()
          return
        }
        // 逐步 +1；禁止一次跳到末尾
        this.displayedCursor += 1
        this.persistWatermarks()
        this.maybeCompleteReplay()
        if (this.uiPhase !== 'replay' && this.uiPhase !== 'offline' && this.uiPhase !== 'error') {
          this.refreshPhase()
        }
        this.enterDoneIfEligible()
        if (this.displayedCursor < this.roundCursor && !this.displayFrozen) {
          const delay = stepIntervalMs(this.roundCursor - this.displayedCursor)
          this._timer = setTimeout(tick, delay)
        }
      }
      const backlog = this.roundCursor - this.displayedCursor
      // 验证入队步进数（测试可依赖 enqueueSteps）
      void enqueueSteps(this.displayedCursor, this.roundCursor)
      this._timer = setTimeout(tick, stepIntervalMs(backlog))
    },

    stopAnimationTimer(): void {
      if (this._timer != null) {
        clearTimeout(this._timer)
        this._timer = null
      }
    },

    refreshPhase(): void {
      if (this.uiPhase === 'error' && this.lastError) {
        return
      }
      if (this.uiPhase === 'offline' || this.uiPhase === 'connecting') {
        return
      }
      if (this.uiPhase === 'replay') {
        return
      }
      if (this.uiPhase === 'done') {
        // 权威仍 completed 才粘住；restart/新轮 in_progress 必须离开 done
        if (this.roundState === 'completed' && !this.pendingCompletion) {
          return
        }
      }
      if (this.displayedCursor === 0 && this.roundCursor === 0) {
        this.uiPhase = 'empty'
      } else {
        this.uiPhase = 'live'
      }
    },

    /** 登出时清展示态 + 三水位键 + 归档。 */
    reset(): void {
      this.stopLiveSession()
      this.roundId = 0
      this.roundCursor = 0
      this.ackedTotal = 0
      this.snapshotSeq = 0
      this.localTotal = 0
      this.roundState = 'in_progress'
      this.pendingCompletion = false
      this.displayedCursor = 0
      this.lastAppliedSeq = 0
      this.uiPhase = 'empty'
      this.lastError = ''
      this.needsResync = false
      this.pendingDeltas = []
      this.replayTargetCursor = 0
      this.reconnectAttempts = 0
      this.closeOverlayKeepPhase()
      this.celebrationPlayedForRoundId = 0
      this.overlayDismissedForRoundId = 0
      this.archives = []
      this.inflightActionId = null
      this.inflightAction = null
      this.actionBusy = false
      this._hydratedOnce = false
      this._flushingPending = false
      clearReadingWatermarks()
    },
  },
})
