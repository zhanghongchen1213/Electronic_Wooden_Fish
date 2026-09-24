/**
 * Story 6.5：完成态 / 礼花门闩 / 弹窗冻结 / round-action 幂等 / 归档护栏。
 */
import { createPinia, setActivePinia } from 'pinia'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { bindUniAdapter } from './api/request'
import { useReadingStreamStore } from './stores/readingStream'
import {
  SYNC_CONTRACT_VERSION,
  type StateSnapshot,
  type WsCompletionFrame,
  type WsDeltaFrame,
} from './types/sync'
import { READING_COPY } from './utils/constants'
import {
  readingBannerCopy,
  readingBannerTitle,
  readingBannerTone,
  readingDoneOverlaySummary,
} from './utils/readingBanner'
import { COUNTS } from './canonical/heart-sutra.generated'
import {
  __resetRealtimeSocketForTests,
  bindConnectSocket,
  type SocketTaskLike,
} from './utils/realtimeSocket'
import { bindUniStorage } from './utils/storage'

const getSyncSnapshot = vi.fn()
const postRoundAction = vi.fn()

vi.mock('./api/sync', () => ({
  getSyncSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  postRoundAction: (...args: unknown[]) => postRoundAction(...args),
  getHistoryStats: vi.fn(async () => ({ code: 0, message: 'ok', data: null })),
}))

function makeSnapshot(partial: Partial<StateSnapshot> = {}): StateSnapshot {
  return {
    device_id: 'dev-1',
    acked_total: 260,
    local_total: 260,
    round_id: 3,
    round_state: 'completed',
    round_cursor: 260,
    pending_completion: false,
    command_revision: 1,
    applied_revision: 1,
    snapshot_seq: 260,
    battery_percent: 80,
    network_mode: 'connected',
    audio_config_version: 1,
    firmware_version: '1.0.0',
    volume: 50,
    brightness: 'mid',
    timeout: 15,
    ...partial,
  }
}

function makeDelta(
  partial: Partial<WsDeltaFrame> & Pick<WsDeltaFrame, 'seq' | 'round_cursor'>,
): WsDeltaFrame {
  return {
    contract_version: SYNC_CONTRACT_VERSION,
    type: 'delta',
    acked_total: 261,
    local_total: 261,
    round_id: 3,
    ...partial,
  }
}

function makeCompletion(partial: Partial<WsCompletionFrame> = {}): WsCompletionFrame {
  return {
    contract_version: SYNC_CONTRACT_VERSION,
    type: 'completion',
    seq: 261,
    round_id: 3,
    round_state: 'completed',
    pending_completion: false,
    ...partial,
  }
}

describe('readingStream DONE 护栏 (Story 6.5)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    __resetRealtimeSocketForTests()
    getSyncSnapshot.mockReset()
    postRoundAction.mockReset()
    const mem = new Map<string, unknown>()
    bindUniStorage({
      getStorageSync: (k) => mem.get(k) ?? '',
      setStorageSync: (k, v) => {
        mem.set(k, v)
      },
      removeStorageSync: (k) => {
        mem.delete(k)
      },
    })
    mem.set('ewf_access_token', 'test-token')
    mem.set('ewf_refresh_token', 'r')
    mem.set('ewf_token_expire_time', Date.now() + 3600_000)
    bindUniAdapter({
      request: () => undefined,
      reLaunch: () => undefined,
      showToast: () => undefined,
      getCurrentPages: () => [],
    })
    bindConnectSocket(() => {
      const task: SocketTaskLike = {
        onOpen: () => undefined,
        onMessage: () => undefined,
        onError: () => undefined,
        onClose: () => undefined,
        send: () => undefined,
        close: () => undefined,
      }
      return task
    })
  })

  afterEach(() => {
    useReadingStreamStore().reset()
    __resetRealtimeSocketForTests()
    vi.restoreAllMocks()
  })

  it('pending_completion=true 不弹窗、不声称本轮完成', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ pending_completion: true }), { animate: false })
    expect(store.pendingCompletion).toBe(true)
    expect(store.overlayVisible).toBe(false)
    expect(store.uiPhase).not.toBe('done')
    expect(readingBannerTitle(store.uiPhase)).not.toBe(READING_COPY.doneBannerTitle)
  })

  it('completed + pending=false 冷启动弹一次 + 礼花门闩', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot(), { animate: false })
    expect(store.uiPhase).toBe('done')
    expect(store.overlayVisible).toBe(true)
    expect(store.celebrationPending).toBe(true)
    expect(store.celebrationPlayedForRoundId).toBe(3)
    expect(store.roundCursor).toBe(COUNTS.consumableHan)

    // 重复 completion 不重复礼花
    store.celebrationPending = false
    store.handleWsFrame(makeCompletion({ seq: 262 }))
    expect(store.overlayVisible).toBe(true)
    expect(store.celebrationPending).toBe(false)
    expect(store.celebrationPlayedForRoundId).toBe(3)
  })

  it('offline / replay 中不误弹；replay 追上后弹', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_state: 'in_progress',
      round_cursor: 10,
      pending_completion: false,
      snapshot_seq: 10,
    }), { animate: false })
    store.uiPhase = 'offline'
    store.roundState = 'completed'
    store.pendingCompletion = false
    store.displayedCursor = 10
    store.roundCursor = 10
    store.enterDoneIfEligible()
    expect(store.overlayVisible).toBe(false)

    store.uiPhase = 'replay'
    store.replayTargetCursor = 10
    store.enterDoneIfEligible()
    expect(store.overlayVisible).toBe(false)

    store.uiPhase = 'live'
    store.enterDoneIfEligible()
    expect(store.uiPhase).toBe('done')
    expect(store.overlayVisible).toBe(true)
  })

  it('弹窗期 delta 不追加正文', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_state: 'in_progress',
      round_cursor: 5,
      pending_completion: false,
      snapshot_seq: 5,
      acked_total: 5,
      local_total: 5,
    }), { animate: false })
    store.roundState = 'completed'
    store.pendingCompletion = false
    store.displayedCursor = 5
    store.roundCursor = 5
    store.lastAppliedSeq = 5
    store.enterDoneIfEligible()
    expect(store.overlayVisible).toBe(true)
    expect(store.displayFrozen).toBe(true)

    const before = store.displayedCursor
    const result = store.applyDelta(makeDelta({
      seq: 6,
      round_cursor: 6,
      round_id: 3,
    }))
    expect(result).toBe('applied')
    expect(store.roundCursor).toBe(6)
    expect(store.displayedCursor).toBe(before)
  })

  it('restart 同 action_id 不双发；成功归档折叠；新轮水合', async () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_cursor: 5,
      round_state: 'completed',
      pending_completion: false,
      snapshot_seq: 5,
      acked_total: 5,
      local_total: 5,
    }), { animate: false })
    expect(store.overlayVisible).toBe(true)

    let calls = 0
    postRoundAction.mockImplementation(async (body: { action: string; action_id: string }) => {
      calls += 1
      expect(body.action).toBe('restart')
      expect(body.action_id).toMatch(/^ewf_ra_restart_/)
      // 模拟慢请求期间二次点击
      await new Promise((r) => setTimeout(r, 20))
      return {
        code: 0,
        message: 'ok',
        data: makeSnapshot({
          round_id: 4,
          round_state: 'in_progress',
          round_cursor: 0,
          pending_completion: false,
          snapshot_seq: 6,
          acked_total: 5,
          local_total: 5,
        }),
      }
    })

    const p1 = store.restartRound()
    const p2 = store.restartRound()
    const [ok1, ok2] = await Promise.all([p1, p2])
    expect(ok1).toBe(true)
    expect(ok2).toBe(false)
    expect(calls).toBe(1)
    expect(store.archives.length).toBe(1)
    expect(store.archives[0]!.roundId).toBe(3)
    expect(store.archives[0]!.collapsed).toBe(true)
    expect(store.roundId).toBe(4)
    expect(store.overlayVisible).toBe(false)
    expect(store.roundState).toBe('in_progress')
    expect(store.uiPhase).not.toBe('done')
  })

  it('失败后同 action_id 重试；弹窗期 round_mismatch 关窗可重开', async () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_cursor: 5,
      round_state: 'completed',
      pending_completion: false,
      snapshot_seq: 5,
      acked_total: 5,
      local_total: 5,
    }), { animate: false })
    expect(store.overlayVisible).toBe(true)

    const ids: string[] = []
    postRoundAction.mockImplementation(async (body: { action: string; action_id: string }) => {
      ids.push(body.action_id)
      if (ids.length === 1) {
        return { code: 20001, message: '暂不可用', data: null }
      }
      return {
        code: 0,
        message: 'ok',
        data: makeSnapshot({
          round_id: 4,
          round_state: 'in_progress',
          round_cursor: 0,
          pending_completion: false,
          snapshot_seq: 6,
          acked_total: 5,
          local_total: 5,
        }),
      }
    })

    expect(await store.restartRound()).toBe(false)
    expect(await store.restartRound()).toBe(true)
    expect(ids).toHaveLength(2)
    expect(ids[0]).toBe(ids[1])
  })

  it('弹窗期 round_mismatch 关窗；同轮 completed 可重开窗不重复礼花', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_cursor: 5,
      round_state: 'completed',
      pending_completion: false,
      snapshot_seq: 5,
      acked_total: 5,
      local_total: 5,
    }), { animate: false })
    expect(store.overlayVisible).toBe(true)
    store.celebrationPending = false

    const result = store.applyDelta(makeDelta({
      seq: 6,
      round_cursor: 1,
      round_id: 99,
    }))
    expect(result).toBe('round_mismatch')
    expect(store.overlayVisible).toBe(false)
    expect(store.needsResync).toBe(true)

    store.enterDoneIfEligible()
    expect(store.overlayVisible).toBe(true)
    expect(store.celebrationPending).toBe(false)
    expect(store.celebrationPlayedForRoundId).toBe(3)
  })

  it('DONE 断线可重连关闭码仍保 done 相位', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    type CloseHandler = (info: { code?: number; reason?: string }) => void
    const closeRef = { current: null as CloseHandler | null }
    bindConnectSocket(() => {
      const handlers: { close?: CloseHandler } = {}
      const task: SocketTaskLike = {
        onOpen: (cb) => {
          queueMicrotask(() => cb())
        },
        onMessage: () => undefined,
        onError: () => undefined,
        onClose: (cb) => {
          handlers.close = cb
          closeRef.current = cb
        },
        send: () => undefined,
        close: () => undefined,
      }
      return task
    })

    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(store.uiPhase).toBe('done')
    expect(closeRef.current).toBeTruthy()

    closeRef.current?.({ code: 1006, reason: 'abnormal' })
    expect(store.uiPhase).toBe('done')
  })

  it('exit 关弹窗保留 completed / done', async () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot(), { animate: false })
    expect(store.overlayVisible).toBe(true)

    postRoundAction.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ pending_completion: false, round_state: 'completed' }),
    })

    const ok = await store.exitRound()
    expect(ok).toBe(true)
    expect(postRoundAction).toHaveBeenCalledWith(
      expect.objectContaining({ action: 'exit', action_id: expect.stringMatching(/^ewf_ra_exit_/) }),
    )
    expect(store.overlayVisible).toBe(false)
    expect(store.uiPhase).toBe('done')
    expect(store.roundState).toBe('completed')
    expect(store.roundId).toBe(3)
    expect(store.displayFrozen).toBe(true)
  })

  it('pending_completion 回真后关已开弹窗', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot(), { animate: false })
    expect(store.overlayVisible).toBe(true)
    store.handleWsFrame(makeCompletion({
      seq: 261,
      pending_completion: true,
      round_state: 'completed',
    }))
    expect(store.pendingCompletion).toBe(true)
    expect(store.overlayVisible).toBe(false)
    expect(store.uiPhase).not.toBe('done')
  })

  it('归档可展开；reset 清空归档', () => {
    const store = useReadingStreamStore()
    store.archives = [{ roundId: 2, chars: ['观', '自'], collapsed: true }]
    store.toggleArchive(2)
    expect(store.archives[0]!.collapsed).toBe(false)
    store.reset()
    expect(store.archives).toEqual([])
  })

  it('DONE banner / overlay 文案对拍；进度分母 260', () => {
    expect(COUNTS.consumableHan).toBe(260)
    expect(readingBannerTone('done')).toBe('done')
    expect(readingBannerTitle('done')).toBe('本轮完成')
    expect(readingBannerCopy('done', 260)).toBe('心经进度 260 / 260 字')
    expect(readingDoneOverlaySummary(260)).toBe('心经进度 260 / 260 字 · 100%')
    expect(READING_COPY.doneRestart).toBe('从头开始')
    expect(READING_COPY.doneExit).toBe('退出')
  })

  it('completion 帧 pending=true 只镜像不弹；随后 false 才弹', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({
      round_state: 'in_progress',
      round_cursor: 260,
      pending_completion: true,
      snapshot_seq: 260,
    }), { animate: false })
    store.handleWsFrame(makeCompletion({
      seq: 261,
      pending_completion: true,
      round_state: 'completed',
    }))
    expect(store.pendingCompletion).toBe(true)
    expect(store.overlayVisible).toBe(false)

    store.handleWsFrame(makeCompletion({
      seq: 262,
      pending_completion: false,
      round_state: 'completed',
    }))
    expect(store.uiPhase).toBe('done')
    expect(store.overlayVisible).toBe(true)
  })
})
