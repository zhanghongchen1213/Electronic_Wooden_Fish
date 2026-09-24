/**
 * Story 6.3 文案/路径扫描：无禁用词；无木鱼点击；仅 request.ts 调 uni.request。
 */
import { readFileSync, readdirSync, statSync } from 'node:fs'
import { join, relative } from 'node:path'
import { fileURLToPath } from 'node:url'
import { describe, expect, it } from 'vitest'
import { READING_COPY } from './utils/constants'

const srcRoot = fileURLToPath(new URL('.', import.meta.url))

const FORBIDDEN = ['TODO', 'draft', 'placeholder', 'data-pencil-id']

function walkFiles(dir: string, acc: string[] = []): string[] {
  for (const name of readdirSync(dir)) {
    if (name === 'node_modules' || name === 'dist' || name.startsWith('.')) {
      continue
    }
    const full = join(dir, name)
    const st = statSync(full)
    if (st.isDirectory()) {
      walkFiles(full, acc)
    } else if (/\.(vue|ts|scss)$/.test(name) && !name.endsWith('.test.ts')) {
      acc.push(full)
    }
  }
  return acc
}

describe('reading guards (Story 6.3)', () => {
  it('冻结 READING 文案', () => {
    expect(READING_COPY.emptyTitle).toBe('等待设备诵读')
    expect(READING_COPY.emptyHint).toBe('尚未有已确认经文')
    expect(READING_COPY.liveBanner).toBe('已同步')
    expect(READING_COPY.replayBannerTitle).toBe('回放中')
    expect(READING_COPY.replayBannerCopy).toBe('新事件排队')
    expect(READING_COPY.offlineBannerTitle).toBe('已断线')
    expect(READING_COPY.offlineBannerCopy).toBe('稍后重试')
    expect(READING_COPY.progressLabel).toBe('心经进度')
  })

  it('阅读页与组件无禁用词、无木鱼点击 handler', () => {
    const targets = walkFiles(srcRoot).filter((f) =>
      /\/(pages\/reading|components\/reading)\//.test(f)
      || f.endsWith('stores/readingStream.ts')
      || f.endsWith('utils/scriptureProjection.ts'),
    )
    const hits: string[] = []
    for (const file of targets) {
      const text = readFileSync(file, 'utf8')
      for (const word of FORBIDDEN) {
        if (text.includes(word)) {
          hits.push(`${relative(srcRoot, file)}:${word}`)
        }
      }
      if (/@click|@tap/.test(text) && /木鱼|wooden.?fish|tapFish|onFish/i.test(text)) {
        hits.push(`${relative(srcRoot, file)}:fish-click`)
      }
    }
    expect(hits).toEqual([])
  })

  it('阅读链路不散落 uni.request；仍仅 request.ts', () => {
    const files = walkFiles(srcRoot)
    const offenders: string[] = []
    for (const file of files) {
      const rel = relative(srcRoot, file)
      if (rel === 'api/request.ts' || rel === join('api', 'request.ts')) {
        continue
      }
      const text = readFileSync(file, 'utf8')
      if (/\buni\.request\b/.test(text)) {
        offenders.push(rel)
      }
    }
    expect(offenders).toEqual([])
  })

  it('types/sync 与 delta 类型面无 glyph/char/confirmed_chars', () => {
    const syncTypes = readFileSync(join(srcRoot, 'types/sync.ts'), 'utf8')
    // 只扫接口字段行，避免注释误伤
    const fieldLines = syncTypes
      .split('\n')
      .filter((line) => /^\s+[a-z_]+\??:/.test(line))
      .join('\n')
    expect(fieldLines).not.toMatch(/\bglyph\b/)
    expect(fieldLines).not.toMatch(/\bconfirmed_chars\b/)
    expect(fieldLines).toContain('round_cursor')
    expect(syncTypes).toContain('interface WsDeltaFrame')
  })
})
