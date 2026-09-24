/**
 * Story 6.7：设备状态页展示缓存（非权威）。
 *
 * 裁决 A：仅 GET /sync/snapshot；不用 /stats 拼、不用 WS 作权威。
 * 裁决 B：pendingCount = max(0, local_total - acked_total)。
 * 裁决 C：applied_revision >= command_revision →「已生效」，否则「待设备应用」。
 * 裁决 D：电量只显示 NN%；禁止「充电/未充电」。
 * 裁决 E：network_mode → 有信号/无信号/已关闭；非法值当无信号。
 * 裁决 F：lastSyncedAt 为客户端成功拉取时刻；失败不得伪造「刚刚」。
 * 裁决 G：刷新 / 立即同步意图 / 重试共用同一单飞 REST；不 POST sync_now。
 * 裁决 H：成功始终 ready（含全 0）；不用全 0 当 empty。
 * Story 6.9 裁决 C/D：queueFull / lowBattery 由快照派生呈现；无独立 wire。
 *
 * 禁止从 readingStream 偷电量/4G 作权威（AD-2）。
 */
import { defineStore } from 'pinia'
import { getSyncSnapshot } from '../api/sync'
import { normalizeToastErrorMessage } from '../api/requestMessage'
import {
  DEVICE_COPY,
  LOW_BATTERY_PERCENT,
  QUEUE_FULL_THRESHOLD,
  SUCCESS_CODE,
} from '../utils/constants'
import type { StateSnapshot } from '../types/sync'

export type DeviceUiPhase = 'loading' | 'empty' | 'ready' | 'fail'

/** sync-action 相位：idle 仅初始；busy=请求在途；ok/pending=成功后按待同步数；fail=失败。 */
export type SyncActionPhase = 'idle' | 'busy' | 'ok' | 'pending' | 'fail'

export type FetchSnapshotReason =
  | 'show'
  | 'pull'
  | 'refresh'
  | 'retry'
  | 'sync_intent'

function pendingFromSnapshot(snap: StateSnapshot): number {
  const local = Number(snap.local_total)
  const acked = Number(snap.acked_total)
  if (!Number.isFinite(local) || !Number.isFinite(acked)) {
    return 0
  }
  return Math.max(0, local - acked)
}

function mapNetworkMode(mode: string): string {
  if (mode === 'connected') {
    return DEVICE_COPY.networkConnected
  }
  if (mode === 'disabled') {
    return DEVICE_COPY.networkDisabled
  }
  // 裁决 E：no_signal 与非法值一律「无信号」，不得抛未捕获异常
  return DEVICE_COPY.networkNoSignal
}

/**
 * 相对短句：成功瞬间「刚刚」；其后「N 分钟前」/「N 小时前」；无记录「尚未同步」。
 * 不引入 moment/dayjs。
 */
export function formatRelativeSyncAt(
  lastSyncedAt: number | null,
  nowMs: number = Date.now(),
): string {
  if (lastSyncedAt == null || lastSyncedAt <= 0) {
    return DEVICE_COPY.lastSyncNever
  }
  const delta = Math.max(0, nowMs - lastSyncedAt)
  if (delta < 60_000) {
    return DEVICE_COPY.lastSyncJustNow
  }
  const minutes = Math.floor(delta / 60_000)
  if (minutes < 60) {
    return `${minutes} 分钟前`
  }
  const hours = Math.floor(minutes / 60)
  if (hours < 48) {
    return `${hours} 小时前`
  }
  const days = Math.floor(hours / 24)
  return `${days} 天前`
}

export const useDeviceStatusStore = defineStore('deviceStatus', {
  state: () => ({
    uiPhase: 'loading' as DeviceUiPhase,
    snapshot: null as StateSnapshot | null,
    /** 客户端成功拉取时刻；非 wire 字段（裁决 F）。 */
    lastSyncedAt: null as number | null,
    syncAction: 'idle' as SyncActionPhase,
    lastError: '' as string,
    refreshing: false,
    /** 单飞锁：show/pull/refresh/retry/sync_intent 共享（裁决 G）。 */
    _inflight: null as Promise<void> | null,
    /** 登出/reset 后作废在途响应。 */
    _fetchGen: 0,
  }),
  getters: {
    pendingCount(state): number {
      if (!state.snapshot) {
        return 0
      }
      return pendingFromSnapshot(state.snapshot)
    },
    /**
     * Story 6.9 裁决 C：积压 >= 1000 → 队列已满（契约同判定输入）。
     * 禁止发明 queue_full wire 字段。
     */
    queueFull(): boolean {
      return this.pendingCount >= QUEUE_FULL_THRESHOLD
    },
    /**
     * Story 6.9 裁决 D：battery_percent <= 20 → 低电。
     * 非有限值不当作低电；禁止充电文案。
     */
    lowBattery(state): boolean {
      if (!state.snapshot) {
        return false
      }
      const raw = state.snapshot.battery_percent
      // null/'' → Number 会变成 0，不得误判为低电
      if (raw == null || raw === '') {
        return false
      }
      const n = Number(raw)
      if (!Number.isFinite(n)) {
        return false
      }
      return n <= LOW_BATTERY_PERCENT
    },
    /** 电量：仅 NN%；禁止充电后缀（裁决 D）。非有限值不渲染 NaN%。 */
    batteryDisplay(state): string {
      if (!state.snapshot) {
        return ''
      }
      const n = Number(state.snapshot.battery_percent)
      if (!Number.isFinite(n)) {
        return DEVICE_COPY.waitingReport
      }
      return `${Math.trunc(n)}%`
    },
    networkDisplay(state): string {
      if (!state.snapshot) {
        return ''
      }
      return mapNetworkMode(state.snapshot.network_mode)
    },
    networkModeKey(state): 'connected' | 'no_signal' | 'disabled' {
      if (!state.snapshot) {
        return 'no_signal'
      }
      const m = state.snapshot.network_mode
      if (m === 'connected' || m === 'disabled') {
        return m
      }
      return 'no_signal'
    },
    /** 裁决 C：applied >= command → 已生效。 */
    commandStatus(state): 'applied' | 'pending' {
      if (!state.snapshot) {
        return 'pending'
      }
      return state.snapshot.applied_revision >= state.snapshot.command_revision
        ? 'applied'
        : 'pending'
    },
    commandStatusDisplay(): string {
      return this.commandStatus === 'applied'
        ? DEVICE_COPY.commandApplied
        : DEVICE_COPY.commandPending
    },
    pendingDisplay(): string {
      return `${this.pendingCount} ${DEVICE_COPY.pendingUnit}`
    },
    /**
     * 相对短句默认用调用时刻；页面应注入 clockMs（见 formatRelativeSyncAt）
     * 以免停留页时永远停在「刚刚」。
     */
    lastSyncDisplay(state): string {
      return formatRelativeSyncAt(state.lastSyncedAt)
    },
    /**
     * 页眉胶囊：待同步数 >0 →「待同步」；否则「已连接」。
     * fail 且无缓存时不展示（none）。
     */
    statusCapsule(state): 'connected' | 'pending' | 'none' {
      if (!state.snapshot) {
        return 'none'
      }
      if (state.uiPhase !== 'ready' && state.uiPhase !== 'fail') {
        return 'none'
      }
      return pendingFromSnapshot(state.snapshot) > 0 ? 'pending' : 'connected'
    },
    /**
     * 同步条短句。pendingCount>0 时禁止伪装「设备与云端一致」。
     * fail 叠加态用失败短句。
     */
    syncBannerCopy(state): string {
      if (state.uiPhase === 'fail') {
        return DEVICE_COPY.syncBannerFail
      }
      if (!state.snapshot) {
        return ''
      }
      if (pendingFromSnapshot(state.snapshot) > 0) {
        return DEVICE_COPY.syncBannerPending
      }
      return DEVICE_COPY.syncBannerOk
    },
    /** 主 CTA：fail → 重试同步；否则「刷新设备状态」。 */
    mainCtaLabel(state): string {
      if (state.uiPhase === 'fail' || state.syncAction === 'fail') {
        return DEVICE_COPY.failAction
      }
      return DEVICE_COPY.mainCta
    },
    /** 弱提示：成功但 firmware_version 空/空白时「等待设备上报」。 */
    weakDeviceHint(state): string {
      if (state.uiPhase !== 'ready' || !state.snapshot) {
        return ''
      }
      const fw = state.snapshot.firmware_version
      if (fw == null || String(fw).trim() === '') {
        return DEVICE_COPY.waitingReport
      }
      return ''
    },
  },
  actions: {
    reset(): void {
      this._fetchGen += 1
      this.uiPhase = 'loading'
      this.snapshot = null
      this.lastSyncedAt = null
      this.syncAction = 'idle'
      this.lastError = ''
      this.refreshing = false
      this._inflight = null
    },

    /**
     * 拉取设备快照。三意图（刷新/立即同步意图/重试）+ pull 共用单飞。
     * reason=pull 时置 refreshing；完成由调用方 stopPullDownRefresh。
     * @returns joined=true 表示并入既有 in-flight，调用方不得再弹「已刷新」。
     */
    async fetchSnapshot(
      opts: { reason: FetchSnapshotReason } = { reason: 'show' },
    ): Promise<{ joined: boolean }> {
      if (this._inflight) {
        await this._inflight
        return { joined: true }
      }

      // 对齐 6.6 裁决 D：以「是否已有成功数据」判定保留，而非 uiPhase===ready
      const retainSnapshot = this.snapshot !== null
      if (opts.reason === 'pull') {
        this.refreshing = true
      } else if (!retainSnapshot) {
        this.uiPhase = 'loading'
      }
      this.syncAction = 'busy'
      this.lastError = ''

      const gen = this._fetchGen
      const pending = this._doFetch(retainSnapshot, gen)
      this._inflight = pending
      try {
        await pending
        return { joined: false }
      } finally {
        if (this._inflight === pending) {
          this._inflight = null
        }
        if (gen === this._fetchGen) {
          this.refreshing = false
        }
      }
    },

    async _doFetch(retainSnapshot: boolean, gen: number): Promise<void> {
      try {
        const body = await getSyncSnapshot()
        if (gen !== this._fetchGen) {
          return
        }
        if (body.code !== SUCCESS_CODE || !body.data) {
          this.lastError =
            normalizeToastErrorMessage(body.message) || DEVICE_COPY.failTitle
          this.uiPhase = 'fail'
          this.syncAction = 'fail'
          if (!retainSnapshot) {
            this.snapshot = null
          }
          return
        }
        const data = body.data
        this.snapshot = {
          device_id: data.device_id,
          acked_total: data.acked_total,
          local_total: data.local_total,
          round_id: data.round_id,
          round_state: data.round_state,
          round_cursor: data.round_cursor,
          pending_completion: data.pending_completion,
          command_revision: data.command_revision,
          applied_revision: data.applied_revision,
          snapshot_seq: data.snapshot_seq,
          battery_percent: data.battery_percent,
          network_mode: data.network_mode,
          audio_config_version: data.audio_config_version,
          firmware_version: data.firmware_version,
          volume: data.volume,
          brightness: data.brightness,
          timeout: data.timeout,
        }
        // 裁决 F：仅成功路径写客户端时刻
        this.lastSyncedAt = Date.now()
        this.lastError = ''
        // 裁决 H：成功始终 ready（含全 0）
        this.uiPhase = 'ready'
        const pending = pendingFromSnapshot(this.snapshot)
        this.syncAction = pending > 0 ? 'pending' : 'ok'
      } catch (err) {
        if (gen !== this._fetchGen) {
          return
        }
        const msg = err instanceof Error ? err.message : String(err)
        this.lastError =
          normalizeToastErrorMessage(msg) || DEVICE_COPY.failTitle
        this.uiPhase = 'fail'
        this.syncAction = 'fail'
        if (!retainSnapshot) {
          this.snapshot = null
        }
      }
    },
  },
})
