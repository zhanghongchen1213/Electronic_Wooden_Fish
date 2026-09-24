/**
 * Story 6.2 裁决 A/B：冷启动与一级页共用鉴权门闸。
 *
 * A：登录成功用 switchTab 到阅读 tab；失败/登出回登录用 reLaunch（5.6 redirectToLogin）。
 * B：App.onLaunch 统一门闸 + 一级 tab onShow 可再调 ensureAuthenticated；内部走 redirectToLogin 判重。
 * E：deviceId 仅 Pinia 内存（不落盘，避免突破 storage 三键白名单）。
 */
import { redirectToLogin } from '../api/request'
import { LOGIN_PAGE_ROUTE, NAV_TAB_ROUTES } from './constants'
import { getAccessToken, getRefreshToken, isTokenExpired } from './storage'

type SwitchTabFn = (options: { url: string }) => void

function defaultSwitchTab(options: { url: string }): void {
  const g = globalThis as { uni?: { switchTab?: SwitchTabFn } }
  if (g.uni?.switchTab) {
    g.uni.switchTab(options)
    return
  }
  throw new Error('uni.switchTab 未绑定')
}

/** 登录成功后直达唯一设备主入口（阅读 tab）。 */
export function navigateToReadingAfterLogin(switchTab: SwitchTabFn = defaultSwitchTab): void {
  switchTab({ url: NAV_TAB_ROUTES.reading })
}

/**
 * 冷启动 / 一级页鉴权。
 * @returns true 表示可留在当前业务页；false 表示已触发（或应触发）回登录。
 */
export function ensureAuthenticated(): boolean {
  const access = getAccessToken()
  const refresh = getRefreshToken()

  if (access && !isTokenExpired()) {
    return true
  }

  // 访问令牌过期但 refresh 仍在：留给 request 单飞静默刷新，不闪登录页
  if (refresh) {
    return true
  }

  redirectToLogin()
  return false
}

/** 是否已在登录页（避免门闸与页面互相跳转）。 */
export function isOnLoginPage(): boolean {
  const g = globalThis as {
    uni?: { getCurrentPages?: () => Array<{ route?: string }> }
  }
  const pages = g.uni?.getCurrentPages?.() ?? []
  const current = pages[pages.length - 1]
  return current?.route === LOGIN_PAGE_ROUTE
}
