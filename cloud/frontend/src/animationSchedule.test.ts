/**
 * Story 6.3：动画调度——Δ 入队、加速不跳号、清空恢复基线。
 */
import { describe, expect, it } from 'vitest'
import {
  ANIM_BASELINE_MS,
  ANIM_MIN_MS,
  enqueueSteps,
  stepIntervalMs,
} from './utils/animationSchedule'

describe('animationSchedule (Story 6.3)', () => {
  it('Δ=5 入队 5 次步进且不跳号', () => {
    const steps = enqueueSteps(10, 15)
    expect(steps).toEqual([11, 12, 13, 14, 15])
    expect(steps.length).toBe(5)
  })

  it('to<=from 不入队', () => {
    expect(enqueueSteps(5, 5)).toEqual([])
    expect(enqueueSteps(8, 3)).toEqual([])
  })

  it('积压加速不跳到 0；清空恢复基线', () => {
    expect(stepIntervalMs(0)).toBe(ANIM_BASELINE_MS)
    expect(stepIntervalMs(1)).toBeLessThan(ANIM_BASELINE_MS)
    expect(stepIntervalMs(1)).toBeGreaterThan(ANIM_MIN_MS)
    expect(stepIntervalMs(8)).toBe(ANIM_MIN_MS)
    expect(stepIntervalMs(100)).toBe(ANIM_MIN_MS)
    expect(ANIM_MIN_MS).toBeGreaterThan(0)
  })
})
