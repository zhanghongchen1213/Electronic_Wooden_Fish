/**
 * Story 6.2：微信小程序登录薄封装。
 * 唯一 HTTP 出口仍是 request.post；页面禁止另起请求通道。
 * 本模块只消费 4.4 已交付的 POST /auth/login/wechat-mini，不改 backend。
 */
import type { ApiResponse, AuthTokenPayload } from '../types/api'
import { post } from './request'

/**
 * 用一次性 wx code 换 JWT。
 * requireAuth:false —— 登录前无 token；showError:false —— 由登录页呈现权限态。
 */
export function loginByWechatMini(code: string): Promise<ApiResponse<AuthTokenPayload>> {
  return post<AuthTokenPayload>(
    '/auth/login/wechat-mini',
    { code },
    { requireAuth: false, showError: false },
  )
}
