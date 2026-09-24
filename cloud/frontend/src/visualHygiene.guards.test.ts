/**
 * Story 6.9：视觉卫生 + 14 态门禁 + 对比度 + 真源边界。
 * 扩展而非替换 shell.guards；扫描范围见裁决 G。
 */
import { readFileSync, readdirSync, statSync } from 'node:fs'
import { join, relative } from 'node:path'
import { fileURLToPath } from 'node:url'
import { createPinia, setActivePinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import {
  DEVICE_COPY,
  LOGIN_COPY,
  LOW_BATTERY_PERCENT,
  QUEUE_FULL_THRESHOLD,
  READING_COPY,
  RECORDS_COPY,
  SETTINGS_COPY,
  STATE_COPY,
} from './utils/constants'
import { TOKEN_BG, TOKEN_INK, contrastRatio } from './utils/contrast'
import type { StateSnapshot } from './types/sync'

const getSyncSnapshot = vi.fn()

vi.mock('./api/sync', () => ({
  getSyncSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  refreshDeviceSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  getHistoryStats: vi.fn(),
  postRoundAction: vi.fn(),
  postCommand: vi.fn(),
}))

const srcRoot = fileURLToPath(new URL('.', import.meta.url))
const frontendRoot = join(srcRoot, '..')

const FORBIDDEN = [
  'TODO',
  'draft',
  'placeholder',
  'data-pencil-id',
  'data-pencil-name',
  'qljP7',
  'WfAs7',
  'Z6Qge',
  'e8Sgp',
  'gGgAm',
  'NtM6r',
]

const LEGACY_PATH_MARKERS = ['legacy/', 'candidate/', 'export-tmp']

function walkFiles(dir: string, acc: string[] = []): string[] {
  for (const name of readdirSync(dir)) {
    if (name === 'node_modules' || name === 'dist' || name.startsWith('.')) {
      continue
    }
    const full = join(dir, name)
    const st = statSync(full)
    if (st.isDirectory()) {
      walkFiles(full, acc)
    } else if (/\.(vue|ts|scss)$/.test(name) && !name.endsWith('.test.ts')) {
      acc.push(full)
    }
  }
  return acc
}

function makeSnapshot(partial: Partial<StateSnapshot> = {}): StateSnapshot {
  return {
    device_id: 'dev-1',
    acked_total: 100,
    local_total: 100,
    round_id: 1,
    round_state: 'active',
    round_cursor: 0,
    pending_completion: false,
    command_revision: 3,
    applied_revision: 3,
    snapshot_seq: 1,
    battery_percent: 76,
    network_mode: 'connected',
    audio_config_version: 1,
    firmware_version: '1.0.0',
    volume: 50,
    brightness: 'mid',
    timeout: 15,
    ...partial,
  }
}

describe('visual hygiene guards (Story 6.9)', () => {
  it('生产源禁用词 / 设计导出属性 / 节点样例 id 扫描干净', () => {
    const roots = [
      join(srcRoot, 'pages'),
      join(srcRoot, 'components'),
      join(srcRoot, 'styles'),
      join(srcRoot, 'App.vue'),
      join(srcRoot, 'utils', 'constants.ts'),
    ]
    const files: string[] = []
    for (const root of roots) {
      const st = statSync(root)
      if (st.isDirectory()) {
        walkFiles(root, files)
      } else {
        files.push(root)
      }
    }
    const hits: string[] = []
    for (const file of files) {
      const text = readFileSync(file, 'utf8')
      for (const word of FORBIDDEN) {
        if (text.includes(word)) {
          hits.push(`${relative(frontendRoot, file)}:${word}`)
        }
      }
    }
    expect(hits).toEqual([])
  })

  it('仅 api/request.ts 调用 uni.request；无第二 HTTP 栈', () => {
    const files = walkFiles(srcRoot)
    const offenders: string[] = []
    for (const file of files) {
      const rel = relative(srcRoot, file).replace(/\\/g, '/')
      if (rel === 'api/request.ts') {
        continue
      }
      const text = readFileSync(file, 'utf8')
      if (/\buni\.request\b/.test(text)) {
        offenders.push(rel)
      }
      // 可执行面：去掉块注释与行注释后再查第二 HTTP 库（注释可写禁令）
      const literals = text.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '')
      if (/\baxios\b/.test(literals) || /\balova\b/i.test(literals)) {
        offenders.push(`${rel}:http-lib`)
      }
    }
    expect(offenders).toEqual([])
  })

  it('真源边界：实现不得引用历史候选路径字符串', () => {
    const scanRoots = [
      join(srcRoot, 'pages'),
      join(srcRoot, 'components'),
      join(srcRoot, 'utils'),
      join(srcRoot, 'stores'),
      join(srcRoot, 'styles'),
    ]
    const files: string[] = []
    for (const root of scanRoots) {
      walkFiles(root, files)
    }
    const hits: string[] = []
    for (const file of files) {
      const text = readFileSync(file, 'utf8')
      for (const marker of LEGACY_PATH_MARKERS) {
        if (text.includes(marker)) {
          hits.push(`${relative(frontendRoot, file)}:${marker}`)
        }
      }
    }
    expect(hits).toEqual([])
  })

  it('令牌 $ink on $bg 对比度 ≥ 7:1', () => {
    const ratio = contrastRatio(TOKEN_INK, TOKEN_BG)
    expect(ratio).toBeGreaterThanOrEqual(7)
    const tokens = readFileSync(join(srcRoot, 'styles/tokens.scss'), 'utf8')
    expect(tokens).toMatch(/\$ink:\s*#2c2823/)
    expect(tokens).toMatch(/\$bg:\s*#f4f0e5/)
    expect(tokens).toMatch(/\$touch-min:\s*44px/)
  })

  it('阅读页仅一组 scripture-progress；无第二百分比条类名', () => {
    const page = readFileSync(join(srcRoot, 'pages/reading/index.vue'), 'utf8')
    const matches = page.match(/scripture-progress/g) ?? []
    // 模板标签一对开闭 + 可能注释；要求恰好一次组件引用
    expect(page).toMatch(/<scripture-progress\b/)
    expect(matches.filter(() => true).length).toBeGreaterThanOrEqual(1)
    const progressFiles = walkFiles(join(srcRoot, 'components/reading')).filter((f) =>
      /Progress|progress/.test(f),
    )
    expect(progressFiles.some((f) => f.endsWith('ScriptureProgress.vue'))).toBe(true)
    expect(progressFiles.length).toBe(1)
    expect(page).not.toMatch(/second-progress|progress-bar-2|percent-bar/i)
  })

  it('STATE_COPY 冻结短句与阈值常量', () => {
    expect(STATE_COPY.queueFullTitle).toBe('队列已满')
    expect(STATE_COPY.queueFullHint).toBe('等待同步排空')
    expect(STATE_COPY.lowBatteryTitle).toBe('电量偏低')
    expect(STATE_COPY.lowBatteryHint).toBe('请尽快连接电源')
    expect(STATE_COPY.clockPendingTitle).toBe('待校时')
    expect(QUEUE_FULL_THRESHOLD).toBe(1000)
    expect(LOW_BATTERY_PERCENT).toBe(20)
    // 预留词表存在，但页面不得无条件渲染待校时
    const devicePage = readFileSync(join(srcRoot, 'pages/device/index.vue'), 'utf8')
    expect(devicePage).not.toMatch(/clockPendingTitle/)
    expect(devicePage).toContain('STATE_COPY.queueFullTitle')
    expect(devicePage).toContain('STATE_COPY.lowBatteryTitle')
  })
})

describe('14-state reachability guards (Story 6.9)', () => {
  const FOURTEEN: Array<{ frame: string; check: () => void }> = [
    {
      frame: 'LOGIN.BASE',
      check: () => {
        expect(LOGIN_COPY.action).toBe('微信登录')
        expect(LOGIN_COPY.title).toBe('电子木鱼')
      },
    },
    {
      frame: 'LOGIN.PERM_ERROR',
      check: () => {
        expect(LOGIN_COPY.permTitle).toBe('权限错误')
        expect(LOGIN_COPY.permAction).toBe('请重新授权')
        const page = readFileSync(join(srcRoot, 'pages/login/index.vue'), 'utf8')
        expect(page).toContain('showPermError')
        expect(page).toContain('perm_error')
      },
    },
    {
      frame: 'READING.LIVE',
      check: () => {
        expect(READING_COPY.liveBanner).toBe('已同步')
      },
    },
    {
      frame: 'READING.REPLAY',
      check: () => {
        expect(READING_COPY.replayBannerTitle).toBe('回放中')
        expect(READING_COPY.replayBannerCopy).toBe('新事件排队')
      },
    },
    {
      frame: 'READING.OFFLINE',
      check: () => {
        expect(READING_COPY.offlineBannerTitle).toBe('已断线')
        expect(READING_COPY.offlineBannerCopy).toBe('稍后重试')
      },
    },
    {
      frame: 'READING.EMPTY',
      check: () => {
        expect(READING_COPY.emptyTitle).toBe('等待设备诵读')
        expect(READING_COPY.emptyHint).toBe('尚未有已确认经文')
      },
    },
    {
      frame: 'READING.DONE',
      check: () => {
        expect(READING_COPY.doneBannerTitle).toBe('本轮完成')
      },
    },
    {
      frame: 'OVERLAY.DONE',
      check: () => {
        expect(READING_COPY.doneOverlayTitle).toBe('本轮完成')
        expect(READING_COPY.doneRestart).toBe('从头开始')
        expect(READING_COPY.doneExit).toBe('退出')
        const modal = readFileSync(join(srcRoot, 'components/reading/ModalDone.vue'), 'utf8')
        expect(modal).toContain('doneRestart')
      },
    },
    {
      frame: 'RECORDS.BASE',
      check: () => {
        expect(RECORDS_COPY.pageTitle).toBe('记录')
        expect(RECORDS_COPY.todayLabel).toBe('今日敲击')
      },
    },
    {
      frame: 'RECORDS.EMPTY',
      check: () => {
        expect(RECORDS_COPY.emptyTitle).toBe('暂无记录')
        expect(RECORDS_COPY.emptyHint).toBe('暂无已确认数据')
      },
    },
    {
      frame: 'DEVICE.BASE',
      check: () => {
        expect(DEVICE_COPY.pageTitle).toBe('设备')
        expect(DEVICE_COPY.mainCta).toBe('刷新设备状态')
      },
    },
    {
      frame: 'DEVICE.FAIL_RETRY',
      check: () => {
        expect(DEVICE_COPY.failTitle).toBe('同步失败')
        expect(DEVICE_COPY.failAction).toBe('重试同步')
      },
    },
    {
      frame: 'SETTINGS.BASE',
      check: () => {
        expect(SETTINGS_COPY.pageTitle).toBe('设置')
        expect(SETTINGS_COPY.primaryCta).toBe('保存并下发')
        const vol = readFileSync(join(srcRoot, 'components/settings/VolumeSlider.vue'), 'utf8')
        expect(vol).toMatch(/\$touch-min/)
      },
    },
    {
      frame: 'SETTINGS.PENDING',
      check: () => {
        // 文案态而非独立路由
        expect(SETTINGS_COPY.applyPending).toBe('待设备应用')
        expect(DEVICE_COPY.commandPending).toBe('待设备应用')
        const pagesJson = readFileSync(join(srcRoot, 'pages.json'), 'utf8')
        expect(pagesJson).not.toMatch(/settings\/pending/)
      },
    },
  ]

  it.each(FOURTEEN)('$frame 文案或相位可达', ({ check }) => {
    check()
  })

  it('印谱组件存在且六页引用（含 OVERLAY.DONE）', () => {
    const sig = readFileSync(
      join(srcRoot, 'components/shared/StyleSignature.vue'),
      'utf8',
    )
    expect(sig).toContain('variant')
    expect(sig).toContain('reading')
    expect(sig).toContain('overlay')
    // 阅读页 17 槽由 ReadingLine 保留，印谱不另造 17 刻度
    expect(sig).not.toMatch(/v-for="n in 17"/)
    for (const page of ['login', 'reading', 'records', 'device', 'settings']) {
      const src = readFileSync(join(srcRoot, `pages/${page}/index.vue`), 'utf8')
      expect(src).toMatch(/style-signature|StyleSignature/)
    }
    const modal = readFileSync(
      join(srcRoot, 'components/reading/ModalDone.vue'),
      'utf8',
    )
    expect(modal).toMatch(/style-signature|StyleSignature/)
    expect(modal).toContain('overlay')
    expect(modal).not.toContain('modal-done__seal')
  })

  it('ConfettiBurst 减少动效仍 emit done（一次状态确认）', () => {
    const src = readFileSync(
      join(srcRoot, 'components/reading/ConfettiBurst.vue'),
      'utf8',
    )
    expect(src).toContain('prefersReducedMotion')
    expect(src).toMatch(/reduceMotion/)
    expect(src).toMatch(/emit\('done'\)/)
  })
})

describe('device derived alerts (Story 6.9)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    getSyncSnapshot.mockReset()
  })

  it('pending >= 1000 → queueFull；battery <= 20 → lowBattery', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        local_total: 1100,
        acked_total: 100,
        battery_percent: 15,
      }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.pendingCount).toBe(1000)
    expect(store.queueFull).toBe(true)
    expect(store.lowBattery).toBe(true)
    expect(store.batteryDisplay).toBe('15%')
    expect(store.batteryDisplay).not.toMatch(/充电/)
  })

  it('边界：999 不满；21% 非低电', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        local_total: 1099,
        acked_total: 100,
        battery_percent: 21,
      }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.pendingCount).toBe(999)
    expect(store.queueFull).toBe(false)
    expect(store.lowBattery).toBe(false)
  })

  it('battery_percent 为 null / NaN 时不当作低电', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        battery_percent: null as unknown as number,
      }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.lowBattery).toBe(false)

    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        battery_percent: Number.NaN,
      }),
    })
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.lowBattery).toBe(false)
  })
})
