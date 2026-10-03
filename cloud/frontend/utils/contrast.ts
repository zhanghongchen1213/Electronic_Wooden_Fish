/**
 * Story 6.9 裁决 H：WCAG 相对亮度与对比度（纯函数）。
 * 正文主色须用 $ink on $bg，禁止用浅灰 $ink-2 承载长段。
 */

function srgbChannelToLinear(c: number): number {
  const v = c / 255
  return v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4
}

/** 解析 #rgb / #rrggbb（忽略 alpha 后缀）。 */
export function parseHexColor(hex: string): { r: number; g: number; b: number } {
  const raw = hex.trim().replace(/^#/, '').slice(0, 6)
  if (raw.length === 3) {
    const r = Number.parseInt(raw[0] + raw[0], 16)
    const g = Number.parseInt(raw[1] + raw[1], 16)
    const b = Number.parseInt(raw[2] + raw[2], 16)
    return { r, g, b }
  }
  if (raw.length !== 6) {
    throw new Error(`invalid hex: ${hex}`)
  }
  return {
    r: Number.parseInt(raw.slice(0, 2), 16),
    g: Number.parseInt(raw.slice(2, 4), 16),
    b: Number.parseInt(raw.slice(4, 6), 16),
  }
}

/** WCAG 相对亮度 L。 */
export function relativeLuminance(hex: string): number {
  const { r, g, b } = parseHexColor(hex)
  const R = srgbChannelToLinear(r)
  const G = srgbChannelToLinear(g)
  const B = srgbChannelToLinear(b)
  return 0.2126 * R + 0.7152 * G + 0.0722 * B
}

/** 对比比 (L1+0.05)/(L2+0.05)，较大亮度在分子。 */
export function contrastRatio(fgHex: string, bgHex: string): number {
  const l1 = relativeLuminance(fgHex)
  const l2 = relativeLuminance(bgHex)
  const lighter = Math.max(l1, l2)
  const darker = Math.min(l1, l2)
  return (lighter + 0.05) / (darker + 0.05)
}

/** 与 tokens.scss 钉死的纸面正文对。 */
export const TOKEN_INK = '#2c2823'
export const TOKEN_BG = '#f4f0e5'
