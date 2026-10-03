/**
 * Story 6.4 裁决 D + 契约 §10.3：断线退避与可重连关闭码。
 *
 * - initial_backoff_ms=1000，max_backoff_ms=30000，max_attempts=10
 * - 指数退避 + full jitter（[0, capped] 均匀采样）
 * - 关闭码 1000 / 1008 不重连；其余可恢复断开走 offline 重试
 */

export const RECONNECT_POLICY = {
  initialBackoffMs: 1000,
  maxBackoffMs: 30000,
  maxAttempts: 10,
} as const

/** 契约：1000 正常关闭、1008 策略拒绝 → 不重连。 */
export function shouldReconnectOnClose(code?: number): boolean {
  if (code === 1000 || code === 1008) {
    return false
  }
  return true
}

/**
 * 计算第 `attempt` 次（0-based）重试前的等待毫秒。
 * full jitter：`random(0 .. min(max, initial * 2^attempt))`
 */
export function nextBackoffMs(
  attempt: number,
  random: () => number = Math.random,
): number {
  const exp = Math.min(
    RECONNECT_POLICY.maxBackoffMs,
    RECONNECT_POLICY.initialBackoffMs * (2 ** Math.max(0, attempt)),
  )
  // random 约定 [0,1)；测试注入若越界则夹紧，保证结果 ∈ [0, exp]
  const unit = Math.min(Math.max(random(), 0), 0.999999999)
  return Math.floor(unit * (exp + 1))
}
