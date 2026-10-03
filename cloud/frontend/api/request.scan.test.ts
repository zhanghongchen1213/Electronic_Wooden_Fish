/**
 * Story 5.6 架构守卫：全仓唯一 HTTP 出口。
 * 2026-10-02 裁决：完全自用测试移除鉴权，原 401 单飞测试一并删除，仅保留 uni.request 垄断扫描。
 */
import { describe, expect, it } from 'vitest'

describe('页面禁止散落 uni.request', () => {
  it('仅 api/request.ts 可调用 uni.request（路径扫描）', async () => {
    const fs = await import('node:fs')
    const path = await import('node:path')
    const root = path.resolve(__dirname, '..')
    const offenders: string[] = []

    function walk(dir: string) {
      for (const name of fs.readdirSync(dir)) {
        const full = path.join(dir, name)
        const st = fs.statSync(full)
        if (st.isDirectory()) {
          if (name === 'node_modules' || name === 'dist' || name === 'unpackage' || name.startsWith('.')) {
            continue
          }
          walk(full)
          continue
        }
        if (!name.endsWith('.ts') || name.endsWith('.test.ts')) {
          continue
        }
        const rel = path.relative(root, full).replace(/\\/g, '/')
        if (rel === 'api/request.ts') {
          continue
        }
        const text = fs.readFileSync(full, 'utf8')
        if (/\buni\.request\b/.test(text) || /\.request\s*\(\s*\{[^}]*url:/.test(text) && text.includes('uni')) {
          // 只禁止字面 uni.request；adapter.request 在 request.ts 内合法
          if (text.includes('uni.request')) {
            offenders.push(rel)
          }
        }
      }
    }
    walk(root)
    expect(offenders).toEqual([])
  })
})
