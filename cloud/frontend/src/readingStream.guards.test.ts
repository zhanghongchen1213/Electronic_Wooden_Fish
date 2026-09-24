/**
 * Story 6.3：WS URL 派生 + 快照/delta 护栏（mock）；无 glyph 字段依赖。
 */
import { createPinia, setActivePinia } from 'pinia'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { bindUniAdapter } from './api/request'
import { useReadingStreamStore } from './stores/readingStream'
import { SYNC_CONTRACT_VERSION, type StateSnapshot, type WsDeltaFrame } from './types/sync'
import {
  __resetRealtimeSocketForTests,
  bindConnectSocket,
  closeRealtimeSocket,
  openRealtimeSocket,
  type SocketTaskLike,
} from './utils/realtimeSocket'
import { bindUniStorage } from './utils/storage'
import { buildRealtimeWsUrl } from './utils/wsUrl'

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

describe('wsUrl (Story 6.3)', () => {
  it('http→ws / https→wss，path 后缀 /ws?token=', () => {
    expect(buildRealtimeWsUrl('tok', 'http://localhost:9218/api/v1')).toBe(
      'ws://localhost:9218/api/v1/ws?token=tok',
    )
    expect(buildRealtimeWsUrl('a b', 'https://example.com/api/v1')).toBe(
      'wss://example.com/api/v1/ws?token=a%20b',
    )
  })
})

describe('readingStream LIVE 护栏 (Story 6.3)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    __resetRealtimeSocketForTests()
    getSyncSnapshot.mockReset()
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
  })

  afterEach(() => {
    useReadingStreamStore().reset()
    __resetRealtimeSocketForTests()
    vi.restoreAllMocks()
  })

  it('先快照水合再连 WS；last_applied_seq = snapshot_seq', async () => {
    const store = useReadingStreamStore()
    const order: string[] = []
    getSyncSnapshot.mockImplementation(async () => {
      order.push('snapshot')
      return { code: 0, message: 'ok', data: makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }) }
    })

    let opened = false
    bindConnectSocket(() => {
      order.push('ws')
      const handlers: {
        open?: () => void
      } = {}
      const task: SocketTaskLike = {
        onOpen: (cb) => {
          handlers.open = cb
        },
        onMessage: () => undefined,
        onError: () => undefined,
        onClose: () => undefined,
        send: () => undefined,
        close: () => undefined,
      }
      queueMicrotask(() => {
        opened = true
        handlers.open?.()
      })
      return task
    })

    await store.startLiveSession()
    expect(order[0]).toBe('snapshot')
    expect(order).toContain('ws')
    expect(store.lastAppliedSeq).toBe(40)
    expect(store.displayedCursor).toBe(5)
    expect(opened).toBe(true)
  })

  it('delta 推进目标游标；落后 seq 丢弃；无 glyph 依赖', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }), { animate: false })
    expect(store.applyDelta(makeDelta({ seq: 40, round_cursor: 6 }))).toBe('stale')
    expect(store.roundCursor).toBe(5)

    const applied = store.applyDelta(makeDelta({
      seq: 41,
      round_cursor: 10,
      acked_total: 45,
    }))
    expect(applied).toBe('applied')
    expect(store.roundCursor).toBe(10)
    expect(store.lastAppliedSeq).toBe(41)
    const frame = makeDelta({ seq: 42, round_cursor: 11 })
    expect('glyph' in frame).toBe(false)
    expect('char' in frame).toBe(false)
  })

  it('seq 空洞触发 needsResync', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }))
    expect(store.applyDelta(makeDelta({ seq: 45, round_cursor: 8 }))).toBe('hole')
    expect(store.needsResync).toBe(true)
  })

  it('lastAppliedSeq=0 时 seq>1 仍判空洞', () => {
    const store = useReadingStreamStore()
    expect(store.lastAppliedSeq).toBe(0)
    expect(store.applyDelta(makeDelta({ seq: 5, round_cursor: 3 }))).toBe('hole')
    expect(store.needsResync).toBe(true)
    expect(store.roundCursor).toBe(0)
  })

  it('round_id 错位返回 round_mismatch 且不推进', () => {
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5, round_id: 1 }))
    expect(store.applyDelta(makeDelta({
      seq: 41,
      round_cursor: 6,
      round_id: 9,
    }))).toBe('round_mismatch')
    expect(store.roundCursor).toBe(5)
    expect(store.needsResync).toBe(true)
  })

  it('delta 动画逐步 ++ 不跳号', async () => {
    vi.useFakeTimers()
    const store = useReadingStreamStore()
    store.applySnapshot(makeSnapshot({ snapshot_seq: 40, round_cursor: 5 }), { animate: false })
    expect(store.displayedCursor).toBe(5)
    store.applyDelta(makeDelta({ seq: 41, round_cursor: 8, acked_total: 41 }))
    expect(store.roundCursor).toBe(8)
    expect(store.displayedCursor).toBe(5)
    await vi.advanceTimersByTimeAsync(320)
    expect(store.displayedCursor).toBe(6)
    await vi.advanceTimersByTimeAsync(400)
    expect(store.displayedCursor).toBe(7)
    await vi.advanceTimersByTimeAsync(400)
    expect(store.displayedCursor).toBe(8)
    vi.useRealTimers()
  })

  it('heartbeat 回 heartbeat_ack', async () => {
    const sent: string[] = []
    bindConnectSocket(() => {
      const handlers: {
        open?: () => void
        message?: (r: { data: string }) => void
      } = {}
      const task: SocketTaskLike = {
        onOpen: (cb) => {
          handlers.open = cb
        },
        onMessage: (cb) => {
          handlers.message = cb as (r: { data: string }) => void
        },
        onError: () => undefined,
        onClose: () => undefined,
        send: (opts) => {
          sent.push(opts.data)
        },
        close: () => undefined,
      }
      queueMicrotask(() => {
        handlers.open?.()
        handlers.message?.({
          data: JSON.stringify({
            contract_version: SYNC_CONTRACT_VERSION,
            type: 'heartbeat',
            seq: 9,
          }),
        })
      })
      return task
    })

    openRealtimeSocket('tok', { onFrame: () => undefined })
    await new Promise((r) => setTimeout(r, 20))
    expect(sent.length).toBe(1)
    const ack = JSON.parse(sent[0]!) as { type: string; seq: number }
    expect(ack.type).toBe('heartbeat_ack')
    expect(ack.seq).toBe(9)
    closeRealtimeSocket()
  })

  it('contract_version 不一致关闭 socket', async () => {
    let closed = false
    const errors: string[] = []
    bindConnectSocket(() => {
      const handlers: {
        open?: () => void
        message?: (r: { data: string }) => void
      } = {}
      const task: SocketTaskLike = {
        onOpen: (cb) => {
          handlers.open = cb
        },
        onMessage: (cb) => {
          handlers.message = cb as (r: { data: string }) => void
        },
        onError: () => undefined,
        onClose: () => undefined,
        send: () => undefined,
        close: () => {
          closed = true
        },
      }
      queueMicrotask(() => {
        handlers.open?.()
        handlers.message?.({
          data: JSON.stringify({
            contract_version: 'WRONG',
            type: 'delta',
            seq: 1,
            acked_total: 1,
            local_total: 1,
            round_id: 1,
            round_cursor: 1,
          }),
        })
      })
      return task
    })

    openRealtimeSocket('tok', {
      onFrame: () => undefined,
      onError: (m) => {
        errors.push(m)
      },
    })
    await new Promise((r) => setTimeout(r, 20))
    expect(errors.some((e) => e.includes('contract_version'))).toBe(true)
    expect(closed).toBe(true)
    closeRealtimeSocket()
  })
})
