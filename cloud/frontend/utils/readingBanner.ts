/**
 * Story 6.4 + 6.5：READING banner 相位 → 文案/tone 映射（对拍 HTML）。
 * 抽成纯函数便于护栏断言，页面不得另写自由句。
 * 裁决 G：DONE banner 优先于 LIVE「已同步」。
 */
import { READING_COPY } from './constants'
import { progressView } from './scriptureProjection'

export type ReadingBannerPhase =
  | 'empty'
  | 'live'
  | 'replay'
  | 'offline'
  | 'connecting'
  | 'error'
  | 'done'

export type ReadingBannerTone = 'live' | 'replay' | 'offline' | 'done'

export function readingBannerTone(phase: ReadingBannerPhase): ReadingBannerTone | null {
  if (phase === 'live') {
    return 'live'
  }
  if (phase === 'replay') {
    return 'replay'
  }
  if (phase === 'offline') {
    return 'offline'
  }
  if (phase === 'done') {
    return 'done'
  }
  return null
}

export function readingBannerTitle(phase: ReadingBannerPhase): string {
  if (phase === 'replay') {
    return READING_COPY.replayBannerTitle
  }
  if (phase === 'offline') {
    return READING_COPY.offlineBannerTitle
  }
  if (phase === 'done') {
    return READING_COPY.doneBannerTitle
  }
  return READING_COPY.liveBanner
}

export function readingBannerCopy(phase: ReadingBannerPhase, cursor = 0): string {
  if (phase === 'replay') {
    return READING_COPY.replayBannerCopy
  }
  if (phase === 'offline') {
    return READING_COPY.offlineBannerCopy
  }
  if (phase === 'done') {
    const view = progressView(cursor)
    return `${READING_COPY.progressLabel} ${view.countLabel}`
  }
  return ''
}

/** OVERLAY 摘要：心经进度 {n} / {total} 字 · 100% */
export function readingDoneOverlaySummary(cursor: number): string {
  const view = progressView(cursor)
  return `${READING_COPY.progressLabel} ${view.countLabel} · 100%`
}
