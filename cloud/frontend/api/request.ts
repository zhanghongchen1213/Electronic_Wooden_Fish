/**
 * Story 5.6 裁决 A/D/E：全仓唯一 HTTP 出口。
 * 2026-10-02 裁决：小程序完全自用测试，移除全部鉴权行为（401 单飞、令牌附加、登录跳转）。
 * 统一保留：信封处理、错误 toast、超时与网络异常兜底。
 */

import type { ApiResponse, RequestOptions } from '../types/api'
import { API_BASE_URL, SERVER_ERROR_CODE, SUCCESS_CODE } from '../utils/constants'
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
  showToast?: (options: { title: string; icon?: string }) => void
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

function toast(title: string): void {
  const adapter = uni()
  if (adapter.showToast && title) {
    adapter.showToast({ title, icon: 'none' })
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

/**
 * 全仓唯一 HTTP 出口。
 */
export async function request<T = unknown>(options: RequestOptions): Promise<ApiResponse<T>> {
  const showError = options.showError !== false
  const method = options.method ?? 'GET'
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options.header ?? {}),
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
