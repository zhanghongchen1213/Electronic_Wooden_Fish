/**
 * Story 5.6 裁决 A/D/E：全仓唯一 HTTP 出口。
 *
 * <p>401 单飞三件套：isRefreshing / refreshSubscribers / onTokenRefreshed|Failed。
 * 失败必须 cb(null) 唤醒；刷新成功后原请求最多重试一次（_retry）；refresh 本身 requireAuth:false。
 * 本模块禁止 console/toast 回显 token 或 session_key。
 */

import type { ApiResponse, AuthTokenPayload, RequestOptions } from '../types/api'
import {
  API_BASE_URL,
  LOGIN_PAGE_PATH,
  LOGIN_PAGE_ROUTE,
  SERVER_ERROR_CODE,
  SUCCESS_CODE,
  UNAUTHORIZED_CODE,
} from '../utils/constants'
import {
  clearTokens,
  getAccessToken,
  getRefreshToken,
  isTokenExpired,
  saveTokens,
} from '../utils/storage'
import { normalizeToastErrorMessage } from './requestMessage'

type UniRequestSuccess = {
  statusCode: number
  data: unknown
}

type UniRequestOptions = {
  url: string
  method: string
  data?: unknown
  header?: Record<string, string>
  timeout?: number
  success: (res: UniRequestSuccess) => void
  fail: (err: { errMsg?: string }) => void
}

export type UniAdapter = {
  request: (options: UniRequestOptions) => void
  reLaunch: (options: { url: string }) => void
  showToast?: (options: { title: string; icon?: string }) => void
  getCurrentPages?: () => Array<{ route?: string }>
}

let uniAdapter: UniAdapter | null = null

/** 测试注入；生产由 bindUniAdapter(uni 包装) 绑定。 */
export function bindUniAdapter(adapter: UniAdapter): void {
  uniAdapter = adapter
}

function uni(): UniAdapter {
  if (uniAdapter) {
    return uniAdapter
  }
  const g = globalThis as { uni?: UniAdapter }
  if (g.uni) {
    return g.uni
  }
  throw new Error('uni adapter 未绑定')
}

let isRefreshing = false
/** 等待刷新完成的请求；入参为 null 表示刷新失败，必须结束等待避免永久挂起 */
let refreshSubscribers: Array<(token: string | null) => void> = []

function subscribeTokenRefresh(cb: (token: string | null) => void): void {
  refreshSubscribers.push(cb)
}

function onTokenRefreshed(newToken: string): void {
  const subs = refreshSubscribers
  refreshSubscribers = []
  subs.forEach((cb) => cb(newToken))
}

/** 刷新失败时唤醒所有排队请求，否则并发请求会永远 await 卡住 */
function onTokenRefreshFailed(): void {
  const subs = refreshSubscribers
  refreshSubscribers = []
  subs.forEach((cb) => cb(null))
}

/** 测试钩子：重置单飞状态。 */
export function __resetRequestStateForTests(): void {
  isRefreshing = false
  refreshSubscribers = []
}

function toast(title: string): void {
  const adapter = uni()
  if (adapter.showToast && title) {
    adapter.showToast({ title, icon: 'none' })
  }
}

export function redirectToLogin(): void {
  const pages = uni().getCurrentPages?.() ?? []
  const current = pages[pages.length - 1]
  if (current?.route === LOGIN_PAGE_ROUTE) {
    return
  }
  uni().reLaunch({ url: LOGIN_PAGE_PATH })
  toast('请重新登录')
}

function unauthorizedBody(message = '登录已过期'): ApiResponse<null> {
  return { code: UNAUTHORIZED_CODE, message, data: null }
}

async function refreshAccessToken(): Promise<string | null> {
  const refreshToken = getRefreshToken()
  if (!refreshToken) {
    clearTokens()
    redirectToLogin()
    return null
  }
  try {
    const body = await rawRequest<AuthTokenPayload>({
      url: '/auth/refresh',
      method: 'POST',
      data: { refreshToken },
      requireAuth: false,
      attachAuth: false,
      showError: false,
    })
    if (body.code === SUCCESS_CODE && body.data?.accessToken && body.data.refreshToken) {
      saveTokens(body.data.accessToken, body.data.refreshToken, body.data.expiresIn ?? 7200)
      return body.data.accessToken
    }
    clearTokens()
    redirectToLogin()
    return null
  } catch {
    clearTokens()
    redirectToLogin()
    return null
  }
}

function normalizeRequestData(data: unknown): unknown {
  if (data === undefined || data === null) {
    return undefined
  }
  if (typeof data === 'string' || data instanceof ArrayBuffer) {
    return data
  }
  return data
}

function performUniRequest(options: UniRequestOptions): Promise<UniRequestSuccess> {
  return new Promise((resolve, reject) => {
    uni().request({
      ...options,
      success: resolve,
      fail: reject,
    })
  })
}

async function rawRequest<T>(options: RequestOptions): Promise<ApiResponse<T>> {
  const requireAuth = options.requireAuth !== false
  const attachAuth = options.attachAuth ?? requireAuth
  const showError = options.showError !== false
  const method = options.method ?? 'GET'
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options.header ?? {}),
  }

  if (attachAuth) {
    const token = getAccessToken()
    if (token) {
      headers.Authorization = `Bearer ${token}`
    }
  }

  const url = options.url.startsWith('http')
    ? options.url
    : `${API_BASE_URL}${options.url.startsWith('/') ? '' : '/'}${options.url}`

  try {
    const res = await performUniRequest({
      url,
      method,
      data: normalizeRequestData(options.data),
      header: headers,
      timeout: 15000,
      success: () => undefined,
      fail: () => undefined,
    })

    const statusCode = res.statusCode
    const body = (res.data ?? {}) as ApiResponse<T>

    if (statusCode === 401 && requireAuth) {
      if (!options._retry) {
        return handleUnauthorized(options)
      }
      // 刷新成功后原请求仍 401：AC2 要求清令牌并只跳转一次登录，禁止带着死 token 继续
      clearTokens()
      redirectToLogin()
      return unauthorizedBody(normalizeToastErrorMessage(body?.message) || '登录已过期') as ApiResponse<T>
    }

    if (statusCode < 200 || statusCode >= 300) {
      const message = normalizeToastErrorMessage(body?.message)
        || `请求失败 (${statusCode})`
      if (showError) {
        toast(message)
      }
      return {
        code: body?.code ?? statusCode * 100,
        message,
        data: null,
      }
    }

    if (body.code !== SUCCESS_CODE) {
      const toastMessage = normalizeToastErrorMessage(body.message)
      if (showError && toastMessage) {
        toast(toastMessage)
      }
      return body
    }

    return body
  } catch (err) {
    const message = err instanceof Error ? err.message : '网络异常，请检查网络连接'
    if (showError) {
      toast('网络异常，请检查网络连接')
    }
    return { code: SERVER_ERROR_CODE, message, data: null }
  }
}

async function handleUnauthorized<T>(options: RequestOptions): Promise<ApiResponse<T>> {
  if (!isRefreshing) {
    isRefreshing = true
    try {
      const newToken = await refreshAccessToken()
      isRefreshing = false
      if (newToken) {
        onTokenRefreshed(newToken)
        return request<T>({ ...options, _retry: true })
      }
      onTokenRefreshFailed()
      return unauthorizedBody() as ApiResponse<T>
    } catch {
      isRefreshing = false
      onTokenRefreshFailed()
      clearTokens()
      redirectToLogin()
      return unauthorizedBody() as ApiResponse<T>
    }
  }

  const token = await new Promise<string | null>((resolve) => {
    subscribeTokenRefresh(resolve)
  })
  if (!token) {
    return unauthorizedBody() as ApiResponse<T>
  }
  return request<T>({ ...options, _retry: true })
}

/**
 * 全仓唯一 HTTP 出口。
 */
export async function request<T = unknown>(options: RequestOptions): Promise<ApiResponse<T>> {
  const requireAuth = options.requireAuth !== false

  if (requireAuth && isTokenExpired()) {
    if (!isRefreshing) {
      isRefreshing = true
      try {
        const newToken = await refreshAccessToken()
        isRefreshing = false
        if (newToken) {
          onTokenRefreshed(newToken)
        } else {
          onTokenRefreshFailed()
          return unauthorizedBody() as ApiResponse<T>
        }
      } catch {
        isRefreshing = false
        onTokenRefreshFailed()
        return unauthorizedBody() as ApiResponse<T>
      }
    } else {
      const token = await new Promise<string | null>((resolve) => {
        subscribeTokenRefresh(resolve)
      })
      if (!token) {
        return unauthorizedBody() as ApiResponse<T>
      }
    }
  }

  return rawRequest<T>(options)
}

export function get<T = unknown>(url: string, options: Omit<RequestOptions, 'url' | 'method'> = {}) {
  return request<T>({ ...options, url, method: 'GET' })
}

export function post<T = unknown>(
  url: string,
  data?: unknown,
  options: Omit<RequestOptions, 'url' | 'method' | 'data'> = {},
) {
  return request<T>({ ...options, url, method: 'POST', data })
}
