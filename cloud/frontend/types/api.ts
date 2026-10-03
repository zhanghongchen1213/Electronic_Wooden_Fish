/**
 * REST 信封与请求选项类型（Story 5.6）。
 * 客户端动作按码表判定，信封不含 retryable 字段。
 */

export interface ApiResponse<T = unknown> {
  code: number
  message: string
  data: T | null
}

export interface RequestOptions {
  url: string
  method?: 'GET' | 'POST' | 'PUT' | 'PATCH' | 'DELETE'
  data?: unknown
  header?: Record<string, string>
  /** 默认 true：业务失败时 toast（滤掉 success） */
  showError?: boolean
}
