/**
 * Story 6.3 裁决 B：动画间隔调度（纯函数，可测）。
 *
 * 基线 320ms；积压时线性降至下限 64ms。
 * 禁止 interval=0 的同步 for 循环「瞬间填满」。
 * 队列深度 = targetCursor - displayedCursor；清空后恢复基线。
 */

/** 基线步进间隔（ms）。 */
export const ANIM_BASELINE_MS = 320

/** 积压加速下限（ms）。 */
export const ANIM_MIN_MS = 64

/** 开始加速的积压深度阈值。 */
export const ANIM_BACKLOG_FULL = 8

/**
 * 按积压深度计算下一步间隔。
 * backlog=0 → 基线；backlog≥ANIM_BACKLOG_FULL → 下限；中间线性插值。
 */
export function stepIntervalMs(backlog: number): number {
  const depth = Math.max(0, Math.floor(backlog))
  if (depth <= 0) {
    return ANIM_BASELINE_MS
  }
  if (depth >= ANIM_BACKLOG_FULL) {
    return ANIM_MIN_MS
  }
  const t = depth / ANIM_BACKLOG_FULL
  return Math.round(ANIM_BASELINE_MS + (ANIM_MIN_MS - ANIM_BASELINE_MS) * t)
}

/**
 * 将目标前进量拆成逐步指针序列（不含起点，含终点）。
 * 例：from=2 to=5 → [3,4,5]；禁止跳号合并。
 */
export function enqueueSteps(fromCursor: number, toCursor: number): number[] {
  const from = Math.max(0, Math.floor(fromCursor))
  const to = Math.max(0, Math.floor(toCursor))
  if (to <= from) {
    return []
  }
  const steps: number[] = []
  for (let c = from + 1; c <= to; c += 1) {
    steps.push(c)
  }
  return steps
}
