/**
 * Story 6.1 壳层护栏：Node 可跑，不依赖微信真机。
 * ① API base 含 /api/v1 ② 四项导航文案 ③ 无用户可见禁用词 ④ 仅 request.ts 调 uni.request
 */
import { readFileSync, readdirSync, statSync } from 'node:fs'
import { join, relative } from 'node:path'
import { fileURLToPath } from 'node:url'
import { describe, expect, it } from 'vitest'
import { API_BASE_URL, LOGIN_PAGE_PATH, LOGIN_PAGE_ROUTE, NAV_TAB_LABELS } from './utils/constants'

const srcRoot = fileURLToPath(new URL('.', import.meta.url))
const frontendRoot = join(srcRoot, '..')

const FORBIDDEN_VISIBLE = ['TODO', 'draft', 'placeholder', 'data-pencil-id']

function walkFiles(dir: string, acc: string[] = []): string[] {
  for (const name of readdirSync(dir)) {
    if (name === 'node_modules' || name === 'dist' || name.startsWith('.')) {
      continue
    }
    const full = join(dir, name)
    const st = statSync(full)
    if (st.isDirectory()) {
      walkFiles(full, acc)
    } else if (/\.(vue|ts|scss|json|html)$/.test(name) && !name.endsWith('.test.ts')) {
      acc.push(full)
    }
  }
  return acc
}

describe('shell guards (Story 6.1)', () => {
  it('VITE_API_BASE_URL / API_BASE_URL 含 /api/v1', () => {
    expect(API_BASE_URL).toContain('/api/v1')
    const envProd = readFileSync(join(frontendRoot, '.env'), 'utf8')
    const envDev = readFileSync(join(frontendRoot, '.env.development'), 'utf8')
    expect(envProd).toMatch(/VITE_API_BASE_URL=.*\/api\/v1/)
    expect(envDev).toMatch(/VITE_API_BASE_URL=.*\/api\/v1/)
  })

  it('四项导航文案固定为 阅读/记录/设备/设置', () => {
    expect([...NAV_TAB_LABELS]).toEqual(['阅读', '记录', '设备', '设置'])
    const pagesJson = readFileSync(join(srcRoot, 'pages.json'), 'utf8')
    for (const label of NAV_TAB_LABELS) {
      expect(pagesJson).toContain(`"text": "${label}"`)
    }
  })

  it('登录路径常量与页面文件一致', () => {
    expect(LOGIN_PAGE_PATH).toBe('/pages/login/index')
    expect(LOGIN_PAGE_ROUTE).toBe('pages/login/index')
    const loginPage = join(srcRoot, 'pages/login/index.vue')
    expect(() => readFileSync(loginPage, 'utf8')).not.toThrow()
  })

  it('用户可见源中无禁用词', () => {
    const files = walkFiles(srcRoot).filter((f) =>
      /\/(pages|components|App\.vue|styles)\//.test(f) || f.endsWith('App.vue'),
    )
    const hits: string[] = []
    for (const file of files) {
      const text = readFileSync(file, 'utf8')
      // 只扫模板与可见样式旁白：粗粒度整文件；注释中的英文词也禁，符合文案卫生预热
      for (const word of FORBIDDEN_VISIBLE) {
        if (text.includes(word)) {
          hits.push(`${relative(frontendRoot, file)}:${word}`)
        }
      }
    }
    expect(hits).toEqual([])
  })

  it('仅 src/api/request.ts 调用 uni.request', () => {
    const files = walkFiles(srcRoot)
    const offenders: string[] = []
    for (const file of files) {
      const rel = relative(srcRoot, file)
      if (rel === join('api', 'request.ts') || rel === 'api/request.ts') {
        continue
      }
      const text = readFileSync(file, 'utf8')
      if (/\buni\.request\b/.test(text)) {
        offenders.push(rel)
      }
    }
    expect(offenders).toEqual([])
    const requestSrc = readFileSync(join(srcRoot, 'api/request.ts'), 'utf8')
    expect(requestSrc).toMatch(/\.request\(/)
  })
})
