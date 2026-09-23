/**
 * Story 5.6 裁决 D/E + Story 6.1 导航常量。
 * 登录页路径固定供 redirectToLogin 与 6.2 消费；本 Story 只建壳/路由。
 * VITE_WX_APPID 仅从 env 读取，禁止硬编码进页面文案或测试快照金值。
 */

/** 单一 API 底座：只读 VITE_API_BASE_URL，禁止第二 base。 */
export const API_BASE_URL: string =
  (typeof import.meta !== 'undefined'
    && import.meta.env
    && typeof import.meta.env.VITE_API_BASE_URL === 'string'
    && import.meta.env.VITE_API_BASE_URL)
  || 'http://localhost:9218/api/v1'

/** 登录失效 reLaunch 目标（Epic 6.1 建壳；6.2 接微信授权）。 */
export const LOGIN_PAGE_PATH = '/pages/login/index'

/** 路由判重用的无前导斜杠形态。 */
export const LOGIN_PAGE_ROUTE = 'pages/login/index'

/** 四项底部导航文案（UX-DR16）；文案固定不可改名。 */
export const NAV_TAB_LABELS = ['阅读', '记录', '设备', '设置'] as const

export type PrimaryTabKey = 'reading' | 'records' | 'device' | 'settings'

export const NAV_TAB_KEYS: PrimaryTabKey[] = ['reading', 'records', 'device', 'settings']

export const NAV_TAB_ROUTES: Record<PrimaryTabKey, string> = {
  reading: '/pages/reading/index',
  records: '/pages/records/index',
  device: '/pages/device/index',
  settings: '/pages/settings/index',
}

/** 微信 AppID：仅 env，部署前替换 replace 占位。 */
export const WX_APPID: string =
  (typeof import.meta !== 'undefined'
    && import.meta.env
    && typeof import.meta.env.VITE_WX_APPID === 'string'
    && import.meta.env.VITE_WX_APPID)
  || ''

export const SUCCESS_CODE = 0
export const UNAUTHORIZED_CODE = 40101
export const SERVER_ERROR_CODE = 50000
