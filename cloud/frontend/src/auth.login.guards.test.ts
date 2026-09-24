/**
 * Story 6.2 登录直达与权限态护栏（Node/vitest，不依赖真机微信）。
 * ① 成功 saveTokens + switchTab reading
 * ② 失败 perm_error 且不写 token
 * ③ 无绑定/迁移路由
 * ④ 可见文案无禁用词
 * ⑤ AuthTokenPayload 含 deviceId
 * ⑥ 登录请求 requireAuth: false
 */
import { readFileSync, readdirSync, statSync } from 'node:fs'
import { join, relative } from 'node:path'
import { fileURLToPath } from 'node:url'
import { createPinia, setActivePinia } from 'pinia'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { loginByWechatMini } from './api/auth'
import {
  __resetRequestStateForTests,
  bindUniAdapter,
  type UniAdapter,
} from './api/request'
import { useUserAuthStore } from './stores/userAuth'
import type { AuthTokenPayload } from './types/api'
import {
  ensureAuthenticated,
  navigateToReadingAfterLogin,
} from './utils/authGate'
import {
  LOGIN_COPY,
  LOGIN_PAGE_PATH,
  NAV_TAB_ROUTES,
  SUCCESS_CODE,
} from './utils/constants'
import {
  STORAGE_KEYS,
  bindUniStorage,
  clearTokens,
  getAccessToken,
  getRefreshToken,
  saveTokens,
} from './utils/storage'

const srcRoot = fileURLToPath(new URL('.', import.meta.url))
const frontendRoot = join(srcRoot, '..')

const FORBIDDEN_VISIBLE = ['TODO', 'draft', 'placeholder', 'data-pencil-id']
const FORBIDDEN_ROUTES = ['bind', 'migrate', 'binding', 'migration', 'device-select', 'select-device']

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

describe('login guards (Story 6.2)', () => {
  let memory: ReturnType<typeof createMemoryStorage>
  let requestCalls: Array<{
    url: string
    header?: Record<string, string>
    data?: unknown
  }>
  let reLaunchUrls: string[]
  let currentRoute: string
  let loginMode: 'ok' | 'business_error' | 'network'

  beforeEach(() => {
    memory = createMemoryStorage()
    bindUniStorage(memory.api)
    __resetRequestStateForTests()
    clearTokens()
    setActivePinia(createPinia())
    requestCalls = []
    reLaunchUrls = []
    currentRoute = 'pages/reading/index'
    loginMode = 'ok'

    const adapter: UniAdapter = {
      request: (options) => {
        requestCalls.push({
          url: options.url,
          header: options.header,
          data: options.data,
        })
        queueMicrotask(() => {
          if (loginMode === 'network') {
            options.fail({ errMsg: 'network fail' })
            return
          }
          if (loginMode === 'business_error') {
            options.success({
              statusCode: 200,
              data: {
                code: 40300,
                message: '身份不匹配',
                data: null,
              },
            })
            return
          }
          options.success({
            statusCode: 200,
            data: {
              code: SUCCESS_CODE,
              message: 'success',
              data: {
                accessToken: 'access-login',
                refreshToken: 'refresh-login',
                expiresIn: 7200,
                deviceId: 'ewf-device-1',
              } satisfies AuthTokenPayload,
            },
          })
        })
      },
      reLaunch: (options) => {
        reLaunchUrls.push(options.url)
      },
      showToast: () => undefined,
      getCurrentPages: () => [{ route: currentRoute }],
    }
    bindUniAdapter(adapter)
  })

  afterEach(() => {
    vi.restoreAllMocks()
  })

  it('AuthTokenPayload 含 deviceId，类型源无 sessionKey/openId', () => {
    const payload: AuthTokenPayload = {
      accessToken: 'a',
      refreshToken: 'r',
      expiresIn: 1,
      deviceId: 'ewf-x',
    }
    expect(payload.deviceId).toBe('ewf-x')
    const typeSrc = readFileSync(join(srcRoot, 'types/api.ts'), 'utf8')
    expect(typeSrc).toMatch(/deviceId:\s*string/)
    const payloadBlock = typeSrc.slice(typeSrc.indexOf('export interface AuthTokenPayload'))
    expect(payloadBlock).not.toMatch(/^\s*(sessionKey|openId)\s*:/m)
  })

  it('loginByWechatMini 走 requireAuth:false 且无 Authorization', async () => {
    const body = await loginByWechatMini('wx-code-1')
    expect(body.code).toBe(SUCCESS_CODE)
    expect(requestCalls).toHaveLength(1)
    expect(requestCalls[0].url).toContain('/auth/login/wechat-mini')
    expect(requestCalls[0].data).toEqual({ code: 'wx-code-1' })
    expect(requestCalls[0].header?.Authorization).toBeUndefined()
    const authSrc = readFileSync(join(srcRoot, 'api/auth.ts'), 'utf8')
    expect(authSrc).toMatch(/requireAuth:\s*false/)
    expect(authSrc).toMatch(/showError:\s*false/)
  })

  it('成功路径 saveTokens + deviceId + switchTab 阅读', async () => {
    const auth = useUserAuthStore()
    const ok = await auth.loginWithWxCode('wx-ok')
    expect(ok).toBe(true)
    expect(auth.loginPhase).toBe('ok')
    expect(auth.deviceId).toBe('ewf-device-1')
    expect(getAccessToken()).toBe('access-login')
    expect(getRefreshToken()).toBe('refresh-login')
    expect(auth.isLoggedIn).toBe(true)

    const tabs: string[] = []
    navigateToReadingAfterLogin((opts) => {
      tabs.push(opts.url)
    })
    expect(tabs).toEqual([NAV_TAB_ROUTES.reading])
    expect(NAV_TAB_ROUTES.reading).toBe('/pages/reading/index')
  })

  it('失败路径进入 perm_error 且不写 token', async () => {
    loginMode = 'business_error'
    const auth = useUserAuthStore()
    const ok = await auth.loginWithWxCode('wx-bad')
    expect(ok).toBe(false)
    expect(auth.loginPhase).toBe('perm_error')
    expect(auth.permErrorMessage).toBe('身份不匹配')
    expect(getAccessToken()).toBeNull()
    expect(getRefreshToken()).toBeNull()
    expect(auth.deviceId).toBe('')
  })

  it('空 code / 网络失败不写半套 token，文案回落权限错误', async () => {
    const auth = useUserAuthStore()
    expect(await auth.loginWithWxCode('')).toBe(false)
    expect(auth.permErrorMessage).toBe(LOGIN_COPY.permTitle)
    expect(getAccessToken()).toBeNull()

    loginMode = 'network'
    expect(await auth.loginWithWxCode('wx-net')).toBe(false)
    expect(auth.loginPhase).toBe('perm_error')
    expect(getAccessToken()).toBeNull()
  })

  it('clearSession 与 logout 成对清空 token 与 deviceId', async () => {
    const auth = useUserAuthStore()
    await auth.loginWithWxCode('wx-ok')
    auth.logout()
    expect(getAccessToken()).toBeNull()
    expect(auth.deviceId).toBe('')
    expect(auth.loginPhase).toBe('idle')
  })

  it('logout 清空 recordsStats 展示缓存', async () => {
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const records = useRecordsStatsStore()
    records.uiPhase = 'ready'
    records.stats = {
      today_taps: 3,
      last_7_days_taps: 3,
      last_30_days_taps: 3,
      total_taps: 3,
      streak_days: 1,
      empty: false,
      pending_sync: false,
    }
    const auth = useUserAuthStore()
    await auth.loginWithWxCode('wx-ok')
    auth.logout()
    expect(records.stats).toBeNull()
    expect(records.uiPhase).toBe('loading')
  })

  it('logout 清空 deviceStatus 展示缓存', async () => {
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const device = useDeviceStatusStore()
    device.uiPhase = 'ready'
    device.lastSyncedAt = Date.now()
    device.syncAction = 'ok'
    device.snapshot = {
      device_id: 'd1',
      acked_total: 1,
      local_total: 1,
      round_id: 1,
      round_state: 'active',
      round_cursor: 0,
      pending_completion: false,
      command_revision: 1,
      applied_revision: 1,
      snapshot_seq: 1,
      battery_percent: 50,
      network_mode: 'connected',
      audio_config_version: 1,
      firmware_version: '1.0.0',
      volume: 50,
      brightness: 'mid',
      timeout: 15,
    }
    const auth = useUserAuthStore()
    await auth.loginWithWxCode('wx-ok')
    auth.logout()
    expect(device.snapshot).toBeNull()
    expect(device.lastSyncedAt).toBeNull()
    expect(device.uiPhase).toBe('loading')
    expect(device.syncAction).toBe('idle')
  })

  it('logout 清空 settingsMirror 展示缓存', async () => {
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const settings = useSettingsMirrorStore()
    settings.uiPhase = 'ready'
    settings.submitPhase = 'fail'
    settings.edit = { volume: 88, brightness: 'high', timeout: 30 }
    settings.snapshot = {
      device_id: 'd1',
      acked_total: 1,
      local_total: 1,
      round_id: 1,
      round_state: 'active',
      round_cursor: 0,
      pending_completion: false,
      command_revision: 2,
      applied_revision: 1,
      snapshot_seq: 3,
      battery_percent: 50,
      network_mode: 'connected',
      audio_config_version: 1,
      firmware_version: '1.0.0',
      volume: 88,
      brightness: 'high',
      timeout: 30,
    }
    const auth = useUserAuthStore()
    await auth.loginWithWxCode('wx-ok')
    auth.logout()
    expect(settings.snapshot).toBeNull()
    expect(settings.edit.volume).toBe(50)
    expect(settings.edit.brightness).toBe('mid')
    expect(settings.edit.timeout).toBe(15)
    expect(settings.uiPhase).toBe('loading')
    expect(settings.submitPhase).toBe('idle')
  })

  it('ensureAuthenticated：无 token 跳登录；有 token 留页；登录页不重复跳', () => {
    expect(ensureAuthenticated()).toBe(false)
    expect(reLaunchUrls).toEqual([LOGIN_PAGE_PATH])

    reLaunchUrls = []
    saveTokens('access-keep', 'refresh-keep', 7200)
    expect(ensureAuthenticated()).toBe(true)
    expect(reLaunchUrls).toEqual([])

    clearTokens()
    currentRoute = 'pages/login/index'
    expect(ensureAuthenticated()).toBe(false)
    expect(reLaunchUrls).toEqual([])
  })

  it('ensureAuthenticated：access 过期但 refresh 仍在则不跳登录', () => {
    saveTokens('access-stale', 'refresh-keep', 7200)
    memory.api.setStorageSync(STORAGE_KEYS.TOKEN_EXPIRE_TIME, Date.now() - 1)
    reLaunchUrls = []
    expect(ensureAuthenticated()).toBe(true)
    expect(reLaunchUrls).toEqual([])
  })

  it('无绑定/迁移路由字符串；login 不在 tabBar', () => {
    const pagesJson = readFileSync(join(srcRoot, 'pages.json'), 'utf8')
    const lower = pagesJson.toLowerCase()
    for (const word of FORBIDDEN_ROUTES) {
      expect(lower).not.toContain(word)
    }
    expect(pagesJson).toContain('"path": "pages/login/index"')
    const tabBlock = pagesJson.slice(pagesJson.indexOf('"tabBar"'))
    expect(tabBlock).not.toContain('pages/login/index')
  })

  it('LOGIN 冻结文案金值存在；可见文案无禁用词', () => {
    expect(LOGIN_COPY.title).toBe('电子木鱼')
    expect(LOGIN_COPY.summary).toBe('查看已确认的心经进度')
    expect(LOGIN_COPY.action).toBe('微信登录')
    expect(LOGIN_COPY.assist).toBe('登录后直达唯一设备')
    expect(LOGIN_COPY.permTitle).toBe('权限错误')
    expect(LOGIN_COPY.permAction).toBe('请重新授权')

    const loginVue = readFileSync(join(srcRoot, 'pages/login/index.vue'), 'utf8')
    expect(loginVue).toContain('login-title')
    expect(loginVue).toContain('login-copy')
    expect(loginVue).toContain('login-action')
    expect(loginVue).toContain('actionBusy')
    expect(loginVue).toMatch(/busy\.value\s*=\s*true/)
    expect(loginVue).not.toContain('请使用微信授权进入')

    const files = walkFiles(srcRoot).filter(
      (f) =>
        /\/(pages|components|App\.vue|styles)\//.test(f) || f.endsWith('App.vue'),
    )
    const hits: string[] = []
    for (const file of files) {
      const text = readFileSync(file, 'utf8')
      for (const word of FORBIDDEN_VISIBLE) {
        if (text.includes(word)) {
          hits.push(`${relative(frontendRoot, file)}:${word}`)
        }
      }
    }
    expect(hits).toEqual([])
  })
})
