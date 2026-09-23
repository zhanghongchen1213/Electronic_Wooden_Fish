/**
 * Story 5.6：401 单飞与安全边界测试（含 RED→GREEN 负例）。
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import {
  __resetRequestStateForTests,
  bindUniAdapter,
  redirectToLogin,
  request,
  type UniAdapter,
} from './request'
import {
  bindUniStorage,
  clearTokens,
  getAccessToken,
  saveTokens,
  STORAGE_KEYS,
} from '../utils/storage'
import { LOGIN_PAGE_PATH, LOGIN_PAGE_ROUTE, SUCCESS_CODE, UNAUTHORIZED_CODE } from '../utils/constants'

type Stored = Record<string, unknown>

function createMemoryStorage() {
  const store: Stored = {}
  return {
    store,
    api: {
      getStorageSync: (key: string) => store[key],
      setStorageSync: (key: string, data: unknown) => {
        store[key] = data
      },
      removeStorageSync: (key: string) => {
        delete store[key]
      },
    },
  }
}

describe('api/request 401 单飞', () => {
  let memory: ReturnType<typeof createMemoryStorage>
  let requestCalls: Array<{ url: string; header?: Record<string, string>; data?: unknown }>
  let refreshCount: number
  let reLaunchCount: number
  let currentRoute: string
  let consoleSpy: ReturnType<typeof vi.spyOn>
  let failRefresh: boolean
  let businessPathHits: number

  beforeEach(() => {
    memory = createMemoryStorage()
    bindUniStorage(memory.api)
    __resetRequestStateForTests()
    requestCalls = []
    refreshCount = 0
    reLaunchCount = 0
    currentRoute = 'pages/reading/index'
    failRefresh = false
    businessPathHits = 0
    consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => undefined)
    vi.spyOn(console, 'error').mockImplementation(() => undefined)
    vi.spyOn(console, 'warn').mockImplementation(() => undefined)

    saveTokens('access-old', 'refresh-1', 7200)

    const adapter: UniAdapter = {
      request: (options) => {
        requestCalls.push({
          url: options.url,
          header: options.header,
          data: options.data,
        })
        const isRefresh = options.url.includes('/auth/refresh')
        if (isRefresh) {
          refreshCount += 1
          queueMicrotask(() => {
            if (failRefresh) {
              options.success({
                statusCode: 401,
                data: { code: UNAUTHORIZED_CODE, message: '刷新失败', data: null },
              })
              return
            }
            options.success({
              statusCode: 200,
              data: {
                code: SUCCESS_CODE,
                message: 'success',
                data: {
                  accessToken: 'access-new',
                  refreshToken: 'refresh-2',
                  expiresIn: 7200,
                  deviceId: 'ewf-test',
                },
              },
            })
          })
          return
        }

        businessPathHits += 1
        const auth = options.header?.Authorization ?? ''
        queueMicrotask(() => {
          if (auth.includes('access-new')) {
            options.success({
              statusCode: 200,
              data: { code: SUCCESS_CODE, message: 'success', data: { ok: true } },
            })
            return
          }
          options.success({
            statusCode: 401,
            data: { code: UNAUTHORIZED_CODE, message: '令牌已过期，请重新登录', data: null },
          })
        })
      },
      reLaunch: () => {
        reLaunchCount += 1
        currentRoute = LOGIN_PAGE_ROUTE
      },
      showToast: () => undefined,
      getCurrentPages: () => [{ route: currentRoute }],
    }
    bindUniAdapter(adapter)
  })

  afterEach(() => {
    consoleSpy.mockRestore()
    clearTokens()
    __resetRequestStateForTests()
  })

  it('并发 3 个 401：只刷新 1 次，全部等待者被唤醒', async () => {
    const results = await Promise.all([
      request({ url: '/sync/snapshot', method: 'GET' }),
      request({ url: '/sync/snapshot', method: 'GET' }),
      request({ url: '/sync/snapshot', method: 'GET' }),
    ])
    expect(refreshCount).toBe(1)
    expect(results.every((r) => r.code === SUCCESS_CODE)).toBe(true)
    expect(getAccessToken()).toBe('access-new')
  })

  it('刷新成功：每个原请求恰好重试 1 次且带新 token', async () => {
    await Promise.all([
      request({ url: '/a', method: 'GET' }),
      request({ url: '/b', method: 'GET' }),
      request({ url: '/c', method: 'GET' }),
    ])
    const businessWithNew = requestCalls.filter(
      (c) => !c.url.includes('/auth/refresh') && c.header?.Authorization === 'Bearer access-new',
    )
    expect(businessWithNew.length).toBe(3)
    expect(refreshCount).toBe(1)
  })

  it('刷新失败：等待者返回未授权；reLaunch 只 1 次', async () => {
    failRefresh = true
    const results = await Promise.all([
      request({ url: '/x', method: 'GET' }),
      request({ url: '/y', method: 'GET' }),
      request({ url: '/z', method: 'GET' }),
    ])
    expect(results.every((r) => r.code === UNAUTHORIZED_CODE)).toBe(true)
    expect(reLaunchCount).toBe(1)
    expect(getAccessToken()).toBeNull()
  })

  it('已在登录页时 reLaunch 为 0', async () => {
    failRefresh = true
    currentRoute = LOGIN_PAGE_ROUTE
    await request({ url: '/x', method: 'GET' })
    expect(reLaunchCount).toBe(0)
  })

  it('负例①：每 401 各刷一次会被检测为 refreshCount>1（单飞防止）', async () => {
    await Promise.all([
      request({ url: '/1', method: 'GET' }),
      request({ url: '/2', method: 'GET' }),
      request({ url: '/3', method: 'GET' }),
    ])
    // GREEN：单飞保证 === 1；若实现退化成每请求各刷，本断言失败
    expect(refreshCount).toBe(1)
  })

  it('负例②：刷新失败必须 cb(null) 唤醒（不得挂起）', async () => {
    failRefresh = true
    const timed = Promise.race([
      Promise.all([
        request({ url: '/1', method: 'GET' }),
        request({ url: '/2', method: 'GET' }),
      ]),
      new Promise<never>((_, reject) => {
        setTimeout(() => reject(new Error('挂起：刷新失败未唤醒等待者')), 2000)
      }),
    ])
    await expect(timed).resolves.toBeTruthy()
  })

  it('负例③：重试不超过一次（_retry 防回路）', async () => {
    let authAttempts = 0
    bindUniAdapter({
      request: (options) => {
        requestCalls.push({ url: options.url, header: options.header })
        if (options.url.includes('/auth/refresh')) {
          refreshCount += 1
          queueMicrotask(() => {
            options.success({
              statusCode: 200,
              data: {
                code: SUCCESS_CODE,
                message: 'success',
                data: { accessToken: 'access-new', refreshToken: 'r2', expiresIn: 7200 },
              },
            })
          })
          return
        }
        authAttempts += 1
        queueMicrotask(() => {
          // 即便带着新 token 仍回 401，也不得再次刷新
          options.success({
            statusCode: 401,
            data: { code: UNAUTHORIZED_CODE, message: '仍失败', data: null },
          })
        })
      },
      reLaunch: () => {
        reLaunchCount += 1
      },
      getCurrentPages: () => [{ route: currentRoute }],
    })

    const result = await request({ url: '/once', method: 'GET' })
    expect(refreshCount).toBe(1)
    expect(authAttempts).toBe(2) // 原始 + 重试 1 次
    expect(result.code).toBe(UNAUTHORIZED_CODE)
    expect(getAccessToken()).toBeNull()
    expect(reLaunchCount).toBe(1)
  })

  it('负例④：不得把 token/session_key 写入 console 或非白名单 storage', async () => {
    await request({ url: '/sync/snapshot', method: 'GET' })
    const logged = consoleSpy.mock.calls.map((c) => String(c.join(' '))).join('\n')
    expect(logged).not.toContain('access-')
    expect(logged).not.toContain('refresh-')
    expect(logged.toLowerCase()).not.toContain('session_key')
    expect(memory.store).not.toHaveProperty('session_key')
    expect(memory.store).not.toHaveProperty('sessionKey')
    expect(Object.keys(memory.store).every((k) => k.startsWith('ewf_'))).toBe(true)
    expect(memory.store[STORAGE_KEYS.ACCESS_TOKEN]).toBe('access-new')
  })

  it('redirectToLogin 目标路径稳定', () => {
    currentRoute = 'pages/device/index'
    redirectToLogin()
    expect(reLaunchCount).toBe(1)
    // reLaunch 由 adapter 计数；路径常量契约
    expect(LOGIN_PAGE_PATH).toBe('/pages/login/index')
  })
})

describe('页面禁止散落 uni.request', () => {
  it('仅 src/api/request.ts 可调用 uni.request（路径扫描）', async () => {
    const fs = await import('node:fs')
    const path = await import('node:path')
    const root = path.resolve(__dirname, '..')
    const offenders: string[] = []

    function walk(dir: string) {
      for (const name of fs.readdirSync(dir)) {
        const full = path.join(dir, name)
        const st = fs.statSync(full)
        if (st.isDirectory()) {
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
