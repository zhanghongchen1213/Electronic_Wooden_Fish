/**
 * Story 6.3 裁决 D：单一 SocketTask 实时客户端。
 *
 * - uni.connectSocket + complete 拿 task；禁止全局 onSocket* 多连接。
 * - 新连接建立前必须关闭旧连接（契约 §10.2）。
 * - 帧 JSON 非 {code,message,data} 信封；心跳：收 heartbeat 回 heartbeat_ack。
 */
import {
  SYNC_CONTRACT_VERSION,
  type WsHeartbeatAckFrame,
  type WsInboundFrame,
} from '../types/sync'
import { buildRealtimeWsUrl } from './wsUrl'

export type SocketTaskLike = {
  onOpen: (cb: () => void) => void
  onMessage: (cb: (res: { data: string | ArrayBuffer }) => void) => void
  onError: (cb: (err: { errMsg?: string }) => void) => void
  onClose: (cb: (res: { code?: number; reason?: string }) => void) => void
  send: (options: {
    data: string
    success?: () => void
    fail?: (err: { errMsg?: string }) => void
  }) => void
  close: (options?: { code?: number; reason?: string }) => void
}

export type ConnectSocketFn = (options: {
  url: string
  complete?: () => void
  success?: () => void
  fail?: (err: { errMsg?: string }) => void
}) => SocketTaskLike

export type RealtimeHandlers = {
  onFrame: (frame: WsInboundFrame) => void
  onOpen?: () => void
  onClose?: (info: { code?: number; reason?: string }) => void
  onError?: (message: string) => void
}

let activeTask: SocketTaskLike | null = null
let connectSocketImpl: ConnectSocketFn | null = null
let seqCounter = 1

/** 测试注入 connectSocket；生产读 uni.connectSocket。 */
export function bindConnectSocket(fn: ConnectSocketFn | null): void {
  connectSocketImpl = fn
}

function resolveConnect(): ConnectSocketFn {
  if (connectSocketImpl) {
    return connectSocketImpl
  }
  const g = globalThis as { uni?: { connectSocket?: ConnectSocketFn } }
  if (g.uni?.connectSocket) {
    return g.uni.connectSocket.bind(g.uni) as ConnectSocketFn
  }
  throw new Error('uni.connectSocket 未绑定')
}

export function __resetRealtimeSocketForTests(): void {
  closeRealtimeSocket()
  connectSocketImpl = null
  seqCounter = 1
}

export function isRealtimeSocketOpen(): boolean {
  return activeTask != null
}

/**
 * 建立单一活跃 socket。先关闭旧连接。
 * URL 由 REST base + token 派生。
 */
export function openRealtimeSocket(accessToken: string, handlers: RealtimeHandlers): SocketTaskLike {
  closeRealtimeSocket()
  const url = buildRealtimeWsUrl(accessToken)
  const connect = resolveConnect()
  const task = connect({
    url,
    complete: () => undefined,
  })
  activeTask = task

  task.onOpen(() => {
    handlers.onOpen?.()
  })
  task.onMessage((res) => {
    const raw = typeof res.data === 'string'
      ? res.data
      : new TextDecoder().decode(res.data)
    let parsed: unknown
    try {
      parsed = JSON.parse(raw)
    } catch {
      handlers.onError?.('帧 JSON 解析失败')
      return
    }
    if (!parsed || typeof parsed !== 'object') {
      return
    }
    const frame = parsed as WsInboundFrame
    if (frame.contract_version !== SYNC_CONTRACT_VERSION) {
      handlers.onError?.('contract_version 不一致')
      // 拒绝后关闭连接，避免继续消费未知契约帧
      closeRealtimeSocket()
      return
    }
    if (frame.type === 'heartbeat') {
      sendHeartbeatAck(task, typeof frame.seq === 'number' ? frame.seq : nextSeq())
      return
    }
    handlers.onFrame(frame)
  })
  task.onError((err) => {
    handlers.onError?.(err.errMsg || 'socket 错误')
  })
  task.onClose((info) => {
    if (activeTask === task) {
      activeTask = null
    }
    handlers.onClose?.(info)
  })
  return task
}

export function closeRealtimeSocket(): void {
  if (!activeTask) {
    return
  }
  const task = activeTask
  activeTask = null
  try {
    task.close({ code: 1000, reason: 'client_close' })
  } catch {
    // ignore
  }
}

function nextSeq(): number {
  const s = seqCounter
  seqCounter += 1
  return s
}

function sendHeartbeatAck(task: SocketTaskLike, seq: number): void {
  const ack: WsHeartbeatAckFrame = {
    contract_version: SYNC_CONTRACT_VERSION,
    type: 'heartbeat_ack',
    seq,
  }
  try {
    task.send({ data: JSON.stringify(ack) })
  } catch {
    // ignore send failure; onError/onClose 会触发重连路径
  }
}
