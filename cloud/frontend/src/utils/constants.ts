/**
 * Story 5.6 裁决 D/E + Story 6.1 导航常量 + Story 6.2 登录文案。
 * 登录页路径固定供 redirectToLogin 与登录直达消费。
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

/**
 * LOGIN 帧冻结文案（对拍 HTML；禁止发明替代营销句）。
 * 不得把 VITE_WX_APPID 写进这些常量。
 */
export const LOGIN_COPY = {
  title: '电子木鱼',
  summary: '查看已确认的心经进度',
  action: '微信登录',
  assist: '登录后直达唯一设备',
  permTitle: '权限错误',
  permAction: '请重新授权',
  agreeHint: '请先阅读并同意协议',
  agreementPrefix: '我已阅读并同意',
  userAgreement: '《用户协议》',
  privacyPolicy: '《隐私政策》',
} as const

export const LEGAL_PAGE_PATHS = {
  userAgreement: '/pages/legal/user-agreement',
  privacyPolicy: '/pages/legal/privacy-policy',
} as const

/**
 * READING 冻结文案（对拍 HTML；禁止发明营销替代句）。
 * Story 6.3：空态 / LIVE banner / 进度标签。
 * Story 6.4 裁决 F：REPLAY/OFFLINE 字面来自 HTML 两段标题+说明；
 * 单行合写可用「回放中 · 新事件排队」（对齐 UI 合同），禁止英文/调试句。
 */
export const READING_COPY = {
  emptyTitle: '等待设备诵读',
  emptyHint: '尚未有已确认经文',
  liveBanner: '已同步',
  replayBannerTitle: '回放中',
  replayBannerCopy: '新事件排队',
  offlineBannerTitle: '已断线',
  offlineBannerCopy: '稍后重试',
  /** Story 6.5：DONE banner / OVERLAY 冻结文案（对拍 HTML；禁止发明）。 */
  doneBannerTitle: '本轮完成',
  doneOverlayTitle: '本轮完成',
  doneRestart: '从头开始',
  doneExit: '退出',
  archiveExpand: '展开归档',
  archiveCollapse: '收起归档',
  progressLabel: '心经进度',
  roundPrefix: '第',
  roundSuffix: '次诵读',
} as const

/**
 * RECORDS 冻结文案（对拍 HTML；禁止发明营销替代句）。
 * Story 6.6：五指标 + EMPTY/FAIL + 页眉状态胶囊。
 * 裁决 C：pending_sync 用弱提示「含待同步」，正式数字不加算。
 */
export const RECORDS_COPY = {
  pageTitle: '记录',
  statusConfirmed: '已确认数据',
  statusPending: '含待同步',
  todayLabel: '今日敲击',
  todayUnit: '次',
  weekLabel: '近 7 日',
  monthLabel: '近 30 日',
  totalLabel: '累计敲击',
  totalUnit: '次',
  streakLabel: '连续诵读',
  streakUnit: '天',
  emptyTitle: '暂无记录',
  emptyHint: '暂无已确认数据',
  failTitle: '同步失败',
  failAction: '重试同步',
  loadingHint: '加载中',
  refreshedToast: '已刷新',
} as const

/**
 * DEVICE 冻结文案（对拍 HTML + 产品禁令）。
 * Story 6.7：电量只 `{n}%`，禁止「充电/未充电」后缀（裁决 D）。
 * 立即同步意图与刷新共用同一单飞；不调用不存在的 sync_now（裁决 G）。
 */
export const DEVICE_COPY = {
  pageTitle: '设备',
  statusConnected: '已连接',
  statusPending: '待同步',
  batteryLabel: '电量',
  networkLabel: '4G',
  networkConnected: '有信号',
  networkNoSignal: '无信号',
  networkDisabled: '已关闭',
  lastSyncLabel: '最后同步',
  lastSyncNever: '尚未同步',
  lastSyncJustNow: '刚刚',
  pendingLabel: '待同步',
  pendingUnit: '条',
  commandLabel: '设置状态',
  commandApplied: '已生效',
  commandPending: '待设备应用',
  syncBannerOk: '设备与云端一致',
  syncBannerPending: '有待同步数据',
  syncBannerFail: '同步失败',
  mainCta: '刷新设备状态',
  failAction: '重试同步',
  weakHint: '等待设备状态',
  waitingReport: '等待设备上报',
  loadingHint: '加载中',
  refreshedToast: '已刷新',
  failTitle: '同步失败',
  failHint: '无法获取设备状态，请重试',
} as const

/**
 * SETTINGS 冻结文案（对拍 HTML + EXPERIENCE；禁止发明调试/营销替代句）。
 * Story 6.8：三控件 + 待设备应用/已生效 +「保存并下发」。
 * brightness UI「中」↔ wire `mid`（禁止 medium）。
 */
export const SETTINGS_COPY = {
  pageTitle: '设置',
  headerCapsule: '设备镜像',
  volumeLabel: '音量',
  brightnessLabel: '亮度',
  brightnessLow: '低',
  brightnessMid: '中',
  brightnessHigh: '高',
  timeoutLabel: '自动熄屏',
  timeout5: '5秒',
  timeout15: '15秒',
  timeout30: '30秒',
  applyApplied: '已生效',
  applyPending: '待设备应用',
  primaryCta: '保存并下发',
  ctaBusy: '下发中',
  failRetry: '下发失败，请重试',
  staleHint: '设置已更新，请确认后再次保存',
  loadingHint: '加载中',
  weakEmpty: '等待设备镜像',
  failTitle: '加载失败',
  failAction: '重试',
} as const

/** Story 6.8：与 backend ErrorCode.COMMAND_STALE_REVISION 对齐。 */
export const COMMAND_STALE_REVISION_CODE = 20006

/**
 * Story 6.9 裁决 C/D/E：派生态阈值与预留词表。
 * 队列已满 = max(0, local_total - acked_total) >= 1000（契约同判定输入，非独立 wire）。
 * 低电量 = battery_percent <= 20；禁止「充电中/未充电」产品态。
 * 待校时：仅词表预留，无 trust 信号则不渲染（裁决 E）。
 */
export const QUEUE_FULL_THRESHOLD = 1000
export const LOW_BATTERY_PERCENT = 20

/**
 * 跨页派生态短句（FR-C-018）。禁止营销词与感叹号；既有 *_COPY 金句不变义。
 */
export const STATE_COPY = {
  queueFullTitle: '队列已满',
  queueFullHint: '等待同步排空',
  lowBatteryTitle: '电量偏低',
  lowBatteryHint: '请尽快连接电源',
  /** 预留：无 backend trust 字段前不得渲染横幅（裁决 E）。 */
  clockPendingTitle: '待校时',
  clockPendingHint: '校时后更新今日统计',
} as const
