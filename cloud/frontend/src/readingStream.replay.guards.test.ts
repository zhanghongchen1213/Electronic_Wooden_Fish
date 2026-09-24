/**
 * Story 6.4：断线查询 / 离线回放护栏（REPLAY / OFFLINE / 排队 / 持久化）。
 */
import { createPinia, setActivePinia } from 'pinia'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { bindUniAdapter } from './api/request'
import { useReadingStreamStore } from './stores/readingStream'
import { SYNC_CONTRACT_VERSION, type StateSnapshot, type WsDeltaFrame } from './types/sync'
import { READING_COPY } from './utils/constants'
import {
  nextBackoffMs,
  RECONNECT_POLICY,
  shouldReconnectOnClose,
} from './utils/reconnectPolicy'
import {
  __resetRealtimeSocketForTests,
  bindConnectSocket,
  closeRealtimeSocket,
  isRealtimeSocketOpen,
  type SocketTaskLike,
} from './utils/realtimeSocket'
import {
  bindUniStorage,
  clearTokens,
  getPersistedLastAppliedSeq,
  getPersistedReplayCursor,
  getPersistedSnapshotSeq,
  persistReadingWatermarks,
  STORAGE_KEYS,
} from './utils/storage'
import {
  readingBannerCopy,
  readingBannerTitle,
  readingBannerTone,
} from './utils/readingBanner'

const getSyncSnapshot = vi.fn()

vi.mock('./api/sync', () => ({
  getSyncSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  postRoundAction: vi.fn(async () => ({ code: 0, message: 'ok', data: null })),
  getHistoryStats: vi.fn(async () => ({ code: 0, message: 'ok', data: null })),
}))

function makeSnapshot(partial: Partial<StateSnapshot> = {}): StateSnapshot {
  return {
    device_id: 'dev-1',
    acked_total: 40,
    local_total: 40,
    round_id: 1,
    round_state: 'in_progress',
    round_cursor: 5,
    pending_completion: false,
    command_revision: 1,
    applied_revision: 1,
    snapshot_seq: 40,
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
    acked_total: 41,
    local_total: 41,
    round_id: 1,
    ...partial,
  }
}

type CloseHandler = (info: { code?: number; reason?: string }) => void
type ErrorHandler = (message: string) => void

function bindOpenSocket(refs: {
  onClose?: { current: CloseHandler | null }
  onError?: { current: ErrorHandler | null }
} = {}): void {
  bindConnectSocket(() => {
    const handlers: {
      open?: () => void
      close?: CloseHandler
      error?: (err: { errMsg?: string }) => void
    } = {}
    const task: SocketTaskLike = {
      onOpen: (cb) => {
        handlers.open = cb
      },
      onMessage: () => undefined,
      onError: (cb) => {
        handlers.error = cb
        if (refs.onError) {
          refs.onError.current = (message) => {
            handlers.error?.({ errMsg: message })
          }
        }
      },
      onClose: (cb) => {
        handlers.close = cb
        if (refs.onClose) {
          refs.onClose.current = cb
        }
      },
      send: () => undefined,
      close: () => undefined,
    }
    queueMicrotask(() => handlers.open?.())
    return task
  })
}

describe('reconnectPolicy (Story 6.4)', () => {
  it('1000/1008 不重连；其余可重连', () => {
    expect(shouldReconnectOnClose(1000)).toBe(false)
    expect(shouldReconnectOnClose(1008)).toBe(false)
    expect(shouldReconnectOnClose(1006)).toBe(true)
    expect(shouldReconnectOnClose(undefined)).toBe(true)
  })

  it('退避参数与 full jitter 上界', () => {
    expect(RECONNECT_POLICY.initialBackoffMs).toBe(1000)
    expect(RECONNECT_POLICY.maxBackoffMs).toBe(30000)
    expect(RECONNECT_POLICY.maxAttempts).toBe(10)
    expect(nextBackoffMs(0, () => 1)).toBe(1000)
    expect(nextBackoffMs(0, () => 0)).toBe(0)
    expect(nextBackoffMs(20, () => 1)).toBe(30000)
  })
})

describe('readingStream REPLAY/OFFLINE 护栏 (Story 6.4)', () => {
  let mem: Map<string, unknown>

  beforeEach(() => {
    setActivePinia(createPinia())
    __resetRealtimeSocketForTests()
    getSyncSnapshot.mockReset()
    mem = new Map()
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
  })

  afterEach(() => {
    useReadingStreamStore().reset()
    __resetRealtimeSocketForTests()
    vi.useRealTimers()
    vi.restoreAllMocks()
  })

  it('冻结 REPLAY/OFFLINE 文案对拍 HTML', () => {
    expect(READING_COPY.replayBannerTitle).toBe('回放中')
    expect(READING_COPY.replayBannerCopy).toBe('新事件排队')
    expect(READING_COPY.offlineBannerTitle).toBe('已断线')
    expect(READING_COPY.offlineBannerCopy).toBe('稍后重试')
    expect(READING_COPY.liveBanner).toBe('已同步')
  })

  it('相位 → banner 文案/tone 映射', () => {
    expect(readingBannerTone('replay')).toBe('replay')
    expect(readingBannerTitle('replay')).toBe('回放中')
    expect(readingBannerCopy('replay')).toBe('新事件排队')
    expect(readingBannerTone('offline')).toBe('offline')
    expect(readingBannerTitle('offline')).toBe('已断线')
    expect(readingBannerCopy('offline')).toBe('稍后重试')
    expect(readingBannerTone('live')).toBe('live')
    expect(readingBannerTitle('live')).toBe('已同步')
    expect(readingBannerCopy('live')).toBe('')
    expect(readingBannerTone('connecting')).toBeNull()
  })

  it('持久化三水位读写；logout/clearTokens 清除', () => {
    persistReadingWatermarks({
      snapshotSeq: 40,
      replayCursor: 3,
      lastAppliedSeq: 40,
    })
    expect(getPersistedSnapshotSeq()).toBe(40)
    expect(getPersistedReplayCursor()).toBe(3)
    expect(getPersistedLastAppliedSeq()).toBe(40)
    expect(mem.get(STORAGE_KEYS.SNAPSHOT_SEQ)).toBe(40)
    clearTokens()
    expect(getPersistedSnapshotSeq()).toBeNull()
    expect(getPersistedReplayCursor()).toBeNull()
    expect(getPersistedLastAppliedSeq()).toBeNull()
    expect(mem.get(STORAGE_KEYS.ACCESS_TOKEN)).toBeUndefined()
  })

  it('reset 清除水位键与 pending', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ round_cursor: 5, snapshot_seq: 40 }), { animate: false })
    store.pendingDeltas.push(makeDelta({ seq: 41, round_cursor: 6 }))
    expect(getPersistedReplayCursor()).toBe(5)
    store.reset()
    expect(store.pendingDeltas).toEqual([])
    expect(getPersistedReplayCursor()).toBeNull()
    expect(store.uiPhase).toBe('empty')
  })

  it('有持久化缺口：快照后进入 REPLAY 并逐字追赶', async () => {
    vi.useFakeTimers()
    persistReadingWatermarks({
      snapshotSeq: 30,
      replayCursor: 2,
      lastAppliedSeq: 30,
    })
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }),
    })
    const closeRef = { current: null as CloseHandler | null }
    bindOpenSocket({ onClose: closeRef })

    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(store.uiPhase).toBe('replay')
    expect(store.isReplaying).toBe(true)
    expect(store.displayedCursor).toBe(2)
    expect(store.roundCursor).toBe(5)
    expect(store.lastAppliedSeq).toBe(40)
    expect(store.replayTargetCursor).toBe(5)

    await vi.advanceTimersByTimeAsync(2000)
    expect(store.displayedCursor).toBe(5)
    expect(store.uiPhase).toBe('live')
  })

  it('回放中 delta 入队不插队；结束后按序 flush', async () => {
    vi.useFakeTimers()
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }), { animate: false })
    store.displayedCursor = 2
    store.replayTargetCursor = 5
    store.uiPhase = 'replay'
    store.ensureAnimationTimer()

    store.handleWsFrame(makeDelta({ seq: 41, round_cursor: 7, acked_total: 41 }))
    store.handleWsFrame(makeDelta({ seq: 42, round_cursor: 8, acked_total: 42 }))
    expect(store.pendingDeltas.map((d) => d.seq)).toEqual([41, 42])
    expect(store.roundCursor).toBe(5)
    expect(store.lastAppliedSeq).toBe(40)

    await vi.advanceTimersByTimeAsync(3000)
    expect(store.displayedCursor).toBeGreaterThanOrEqual(5)
    expect(store.pendingDeltas.length).toBe(0)
    expect(store.lastAppliedSeq).toBe(42)
    expect(store.roundCursor).toBe(8)
    expect(store.uiPhase).toBe('live')
  })

  it('空洞先 resync；stale 丢弃；旧 round_id 拒写', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5, round_id: 1 }))
    expect(store.applyDelta(makeDelta({ seq: 40, round_cursor: 6 }))).toBe('stale')
    expect(store.applyDelta(makeDelta({ seq: 45, round_cursor: 8 }))).toBe('hole')
    expect(store.needsResync).toBe(true)
    store.needsResync = false
    expect(store.applyDelta(makeDelta({
      seq: 41,
      round_cursor: 6,
      round_id: 9,
    }))).toBe('round_mismatch')
    expect(store.roundCursor).toBe(5)
  })

  it('回放结束 flush 遇空洞时走快照重建，不误标 live', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 50, round_cursor: 5 }),
    })
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }), { animate: false })
    store.uiPhase = 'replay'
    store.replayTargetCursor = 5
    store.displayedCursor = 5
    store.pendingDeltas = [
      makeDelta({ seq: 45, round_cursor: 9 }), // hole relative to lastAppliedSeq=40
    ]
    const callsBefore = getSyncSnapshot.mock.calls.length
    store.flushPendingAfterReplay()
    expect(store.pendingDeltas).toEqual([])
    expect(store.uiPhase).toBe('connecting')
    await Promise.resolve()
    await Promise.resolve()
    expect(getSyncSnapshot.mock.calls.length).toBeGreaterThan(callsBefore)
  })

  it('断线 → offline → 快照补齐（有缺口则 replay）→ live；1000/1008 不重连', async () => {
    vi.useFakeTimers()
    persistReadingWatermarks({
      snapshotSeq: 30,
      replayCursor: 2,
      lastAppliedSeq: 30,
    })
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 30, round_cursor: 2 }),
    })
    const closeRef = { current: null as CloseHandler | null }
    bindOpenSocket({ onClose: closeRef })

    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(store.uiPhase).toBe('live')
    expect(closeRef.current).toBeTruthy()

    // 固定 full jitter→0，使退避立刻触发且不吞掉回放首帧
    const randomSpy = vi.spyOn(Math, 'random').mockReturnValue(0)
    closeRef.current?.({ code: 1006, reason: 'abnormal' })
    expect(store.uiPhase).toBe('offline')
    expect(store.isReplaying).toBe(true)

    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }),
    })
    await vi.advanceTimersByTimeAsync(1)
    // 让 resyncFromSnapshot 的异步链落地
    await Promise.resolve()
    await Promise.resolve()
    await Promise.resolve()
    randomSpy.mockRestore()
    expect(store.uiPhase).toBe('replay')
    expect(store.displayedCursor).toBe(2)
    expect(store.roundCursor).toBe(5)

    await vi.advanceTimersByTimeAsync(3000)
    expect(store.uiPhase).toBe('live')

    closeRef.current?.({ code: 1000, reason: 'normal' })
    expect(store.uiPhase).toBe('error')
    expect(store.reconnectAttempts).toBeLessThanOrEqual(RECONNECT_POLICY.maxAttempts)

    store.uiPhase = 'live'
    store.lastError = ''
    closeRef.current?.({ code: 1008, reason: 'policy' })
    expect(store.uiPhase).toBe('error')
  })

  it('socket onError：可恢复走 offline；contract_version 进 error 不重连', async () => {
    vi.useFakeTimers()
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }),
    })
    const closeRef = { current: null as CloseHandler | null }
    const errorRef = { current: null as ErrorHandler | null }
    bindOpenSocket({ onClose: closeRef, onError: errorRef })

    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(store.uiPhase).toBe('live')
    expect(errorRef.current).toBeTruthy()

    // 帧解析类错误且 socket 仍开：只记错，不立刻 offline
    errorRef.current?.('帧 JSON 解析失败')
    expect(store.uiPhase).toBe('live')
    expect(store.lastError).toBe('帧 JSON 解析失败')

    closeRealtimeSocket()
    expect(isRealtimeSocketOpen()).toBe(false)
    errorRef.current?.('socket 错误')
    expect(store.uiPhase).toBe('offline')

    store.uiPhase = 'live'
    store.lastError = ''
    errorRef.current?.('contract_version 不一致')
    expect(store.uiPhase).toBe('error')
  })

  it('快照失败进入 offline 退避；不盲连 WS', async () => {
    vi.useFakeTimers()
    getSyncSnapshot.mockResolvedValue({
      code: 50001,
      message: '快照不可用',
      data: null,
    })
    const closeRef = { current: null as CloseHandler | null }
    bindOpenSocket({ onClose: closeRef })
    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(store.uiPhase).toBe('offline')
    expect(isRealtimeSocketOpen()).toBe(false)
    expect(store.reconnectAttempts).toBe(1)
  })

  it('liveActive 已 true 时 startLiveSession 仍幂等对齐水位', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }),
    })
    const closeRef = { current: null as CloseHandler | null }
    bindOpenSocket({ onClose: closeRef })
    const store = useReadingStreamStore()
    await store.startLiveSession()
    expect(getSyncSnapshot).toHaveBeenCalledTimes(1)

    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 50, round_cursor: 8, acked_total: 50 }),
    })
    await store.startLiveSession()
    expect(getSyncSnapshot).toHaveBeenCalledTimes(2)
    expect(store.snapshotSeq).toBe(50)
    expect(store.roundCursor).toBe(8)
  })

  it('步进时持久化 replay_cursor；权威 snapshot_seq 来自快照', async () => {
    vi.useFakeTimers()
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }), { animate: false })
    expect(getPersistedSnapshotSeq()).toBe(40)
    expect(getPersistedReplayCursor()).toBe(5)
    store.applyDelta(makeDelta({ seq: 41, round_cursor: 7, acked_total: 41 }))
    expect(getPersistedLastAppliedSeq()).toBe(41)
    await vi.advanceTimersByTimeAsync(800)
    expect(getPersistedReplayCursor()).toBe(store.displayedCursor)
    expect(getPersistedSnapshotSeq()).toBe(40)
  })
})
