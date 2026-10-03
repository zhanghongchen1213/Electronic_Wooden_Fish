/**
 * Story 6.3 裁决 A/E：游标 → 字形 / 17 槽布局 / 进度视图（纯函数）。
 *
 * - round_cursor = N → STEPS.slice(0,N).map(s => s.display) 再按码点切槽；
 * - 标点计槽；进度分母 = COUNTS.consumableHan（260），不是 totalChars 303；
 * - 禁止预览未来经文；不越 STEPS.length。
 */
import { COUNTS, STEPS } from '../canonical/heart-sutra.generated'

/** UX-DR15：每行固定 17 槽。 */
export const SLOTS_PER_LINE = 17

export interface ProgressView {
  cursor: number
  total: number
  /** 如 `42 / 260 字` */
  countLabel: string
  /** 保留 1 位小数，如 `16.2`；clamp 0–100 */
  percent: number
  percentLabel: string
}

/**
 * 已确认前 N 个可消费步进的 display 槽字符序列。
 * N=0 → []；N 钳制到 [0, STEPS.length]。
 */
export function displayCharsUpTo(cursor: number): string[] {
  const n = clampCursor(cursor)
  const chars: string[] = []
  for (let i = 0; i < n; i += 1) {
    const display = STEPS[i]?.display ?? ''
    for (const ch of display) {
      chars.push(ch)
    }
  }
  return chars
}

/** 按 slotsPerLine 分行；末行可不足一行。 */
export function layoutRows(chars: readonly string[], slotsPerLine = SLOTS_PER_LINE): string[][] {
  const rows: string[][] = []
  if (slotsPerLine <= 0) {
    return rows
  }
  for (let i = 0; i < chars.length; i += slotsPerLine) {
    rows.push(chars.slice(i, i + slotsPerLine))
  }
  return rows
}

/**
 * 进度视图：分子 = round_cursor（已确认可消费字数），分母 = 260。
 */
export function progressView(cursor: number): ProgressView {
  const n = clampCursor(cursor)
  const total = COUNTS.consumableHan
  const raw = total > 0 ? (n / total) * 100 : 0
  const percent = Math.min(100, Math.max(0, raw))
  const rounded = Math.round(percent * 10) / 10
  return {
    cursor: n,
    total,
    countLabel: `${n} / ${total} 字`,
    percent: rounded,
    percentLabel: `${rounded.toFixed(1)}%`,
  }
}

function clampCursor(cursor: number): number {
  if (!Number.isFinite(cursor) || cursor <= 0) {
    return 0
  }
  const max = STEPS.length
  return Math.min(max, Math.floor(cursor))
}
