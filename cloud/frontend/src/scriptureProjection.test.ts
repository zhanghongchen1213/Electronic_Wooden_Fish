/**
 * Story 6.3：经文投影护栏——17 槽、标点占槽、进度分母 260、前缀稳定。
 */
import { describe, expect, it } from 'vitest'
import { COUNTS, STEPS } from './canonical/heart-sutra.generated'
import {
  displayCharsUpTo,
  layoutRows,
  progressView,
  SLOTS_PER_LINE,
} from './utils/scriptureProjection'

describe('scriptureProjection (Story 6.3)', () => {
  it('cursor=0 → 空槽，不预览未来', () => {
    expect(displayCharsUpTo(0)).toEqual([])
    expect(layoutRows(displayCharsUpTo(0))).toEqual([])
  })

  it('标点计槽：萨， 占两槽', () => {
    // STEPS[4] = { consumable: '萨', display: '萨，' }
    const chars = displayCharsUpTo(5)
    expect(chars.slice(0, 6)).toEqual(['观', '自', '在', '菩', '萨', '，'])
  })

  it('17 槽换行稳定', () => {
    const chars = displayCharsUpTo(20)
    const rows = layoutRows(chars, SLOTS_PER_LINE)
    expect(SLOTS_PER_LINE).toBe(17)
    for (const row of rows.slice(0, -1)) {
      expect(row.length).toBe(17)
    }
    expect(rows.flat().join('')).toBe(chars.join(''))
  })

  it('前缀稳定：增大 cursor 不改写已有前缀', () => {
    const a = displayCharsUpTo(10).join('')
    const b = displayCharsUpTo(20).join('')
    expect(b.startsWith(a)).toBe(true)
  })

  it('进度分母 = consumableHan 260，不是 totalChars 303', () => {
    expect(COUNTS.consumableHan).toBe(260)
    expect(COUNTS.totalChars).toBe(303)
    const view = progressView(42)
    expect(view.total).toBe(260)
    expect(view.countLabel).toBe('42 / 260 字')
    expect(view.percentLabel).toBe('16.2%')
  })

  it('cursor 边界钳制到 STEPS.length，不越界', () => {
    const over = displayCharsUpTo(STEPS.length + 50)
    const full = displayCharsUpTo(STEPS.length)
    expect(over).toEqual(full)
    expect(progressView(-3).cursor).toBe(0)
    expect(progressView(9999).cursor).toBe(STEPS.length)
  })
})
