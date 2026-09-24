/**
 * Story 6.7：设备状态页护栏。
 * ① 字段映射（电量/4G/待同步/命令态）② pending=local-acked ③ 禁充电文案
 * ④ 失败可重试 ⑤ 单飞不双发 ⑥ 禁用词 / 无第二 uni.request
 * ⑦ pending>0 不得伪装「设备与云端一致」为唯一成功态
 */
import { readFileSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { createPinia, setActivePinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { DEVICE_COPY } from './utils/constants'
import {
  formatRelativeSyncAt,
} from './stores/deviceStatus'
import type { StateSnapshot } from './types/sync'

const getSyncSnapshot = vi.fn()

vi.mock('./api/sync', () => ({
  getSyncSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  refreshDeviceSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  getHistoryStats: vi.fn(),
  postRoundAction: vi.fn(),
}))

const srcRoot = fileURLToPath(new URL('.', import.meta.url))

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

describe('deviceStatus store (Story 6.7)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    getSyncSnapshot.mockReset()
  })

  it('字段映射：电量 NN%、4G 文案、待同步、命令态', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        battery_percent: 76,
        network_mode: 'connected',
        local_total: 110,
        acked_total: 100,
        command_revision: 5,
        applied_revision: 3,
      }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.batteryDisplay).toBe('76%')
    expect(store.batteryDisplay).not.toMatch(/充电/)
    expect(store.networkDisplay).toBe('有信号')
    expect(store.pendingCount).toBe(10)
    expect(store.pendingDisplay).toBe('10 条')
    expect(store.commandStatus).toBe('pending')
    expect(store.commandStatusDisplay).toBe('待设备应用')
    expect(store.statusCapsule).toBe('pending')
    expect(store.syncAction).toBe('pending')
    expect(store.syncBannerCopy).toBe(DEVICE_COPY.syncBannerPending)
    expect(store.syncBannerCopy).not.toBe(DEVICE_COPY.syncBannerOk)
  })

  it('network_mode 三态 + 非法值兜底无信号', async () => {
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()

    getSyncSnapshot.mockResolvedValueOnce({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ network_mode: 'no_signal' }),
    })
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.networkDisplay).toBe('无信号')

    getSyncSnapshot.mockResolvedValueOnce({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ network_mode: 'disabled' }),
    })
    await store.fetchSnapshot({ reason: 'refresh' })
    expect(store.networkDisplay).toBe('已关闭')

    getSyncSnapshot.mockResolvedValueOnce({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ network_mode: 'wifi_oops' }),
    })
    await store.fetchSnapshot({ reason: 'refresh' })
    expect(store.networkDisplay).toBe('无信号')
  })

  it('pendingCount = max(0, local - acked)；负差截断为 0', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ local_total: 50, acked_total: 80 }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.pendingCount).toBe(0)
    expect(store.syncAction).toBe('ok')
    expect(store.syncBannerCopy).toBe(DEVICE_COPY.syncBannerOk)
    expect(store.statusCapsule).toBe('connected')
  })

  it('命令态：applied >= command → 已生效', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ command_revision: 2, applied_revision: 2 }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.commandStatus).toBe('applied')
    expect(store.commandStatusDisplay).toBe('已生效')
  })

  it('电量 0% 仍显示 0%；禁止充电后缀常量', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ battery_percent: 0 }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.batteryDisplay).toBe('0%')
    expect(store.uiPhase).toBe('ready')
    const copyBlob = JSON.stringify(DEVICE_COPY)
    expect(copyBlob).not.toMatch(/充电/)
    expect(copyBlob).not.toMatch(/未充电/)
  })

  it('最后同步：成功写 lastSyncedAt；失败不伪造刚刚', async () => {
    expect(formatRelativeSyncAt(null)).toBe(DEVICE_COPY.lastSyncNever)
    const just = Date.now()
    expect(formatRelativeSyncAt(just, just + 1000)).toBe(
      DEVICE_COPY.lastSyncJustNow,
    )
    expect(formatRelativeSyncAt(just - 5 * 60_000, just)).toBe('5 分钟前')

    getSyncSnapshot.mockResolvedValue({
      code: 50000,
      message: '网络错误',
      data: null,
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.uiPhase).toBe('fail')
    expect(store.lastSyncedAt).toBeNull()
    expect(store.lastSyncDisplay).toBe(DEVICE_COPY.lastSyncNever)
    expect(store.syncAction).toBe('fail')
    expect(store.mainCtaLabel).toBe(DEVICE_COPY.failAction)
  })

  it('失败可重试；保留上次 snapshot；主 CTA 切重试文案', async () => {
    getSyncSnapshot
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeSnapshot({ battery_percent: 50 }),
      })
      .mockResolvedValueOnce({
        code: 50000,
        message: '网络错误',
        data: null,
      })
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeSnapshot({ battery_percent: 60 }),
      })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.batteryDisplay).toBe('50%')
    expect(store.mainCtaLabel).toBe(DEVICE_COPY.mainCta)

    await store.fetchSnapshot({ reason: 'retry' })
    expect(store.uiPhase).toBe('fail')
    expect(store.snapshot?.battery_percent).toBe(50)
    expect(store.syncBannerCopy).toBe(DEVICE_COPY.syncBannerFail)
    expect(store.mainCtaLabel).toBe(DEVICE_COPY.failAction)

    await store.fetchSnapshot({ reason: 'retry' })
    expect(store.uiPhase).toBe('ready')
    expect(store.batteryDisplay).toBe('60%')
    expect(store.mainCtaLabel).toBe(DEVICE_COPY.mainCta)
  })

  it('刷新/同步意图/重试/下拉单飞不双发', async () => {
    let resolve!: (v: unknown) => void
    const pending = new Promise((r) => {
      resolve = r
    })
    getSyncSnapshot.mockReturnValue(pending)
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    const a = store.fetchSnapshot({ reason: 'show' })
    const b = store.fetchSnapshot({ reason: 'refresh' })
    const c = store.fetchSnapshot({ reason: 'sync_intent' })
    const d = store.fetchSnapshot({ reason: 'retry' })
    const e = store.fetchSnapshot({ reason: 'pull' })
    expect(store.syncAction).toBe('busy')
    expect(getSyncSnapshot).toHaveBeenCalledTimes(1)
    resolve({ code: 0, message: 'ok', data: makeSnapshot() })
    const results = await Promise.all([a, b, c, d, e])
    expect(getSyncSnapshot).toHaveBeenCalledTimes(1)
    expect(store.uiPhase).toBe('ready')
    expect(store.syncAction).toBe('ok')
    expect(results[0].joined).toBe(false)
    expect(results.slice(1).every((r) => r.joined)).toBe(true)
  })

  it('非有限电量/计数不渲染 NaN；firmware 空白弱提示', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        battery_percent: Number.NaN as unknown as number,
        local_total: Number.NaN as unknown as number,
        acked_total: 10,
        firmware_version: '   ',
      }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.batteryDisplay).toBe(DEVICE_COPY.waitingReport)
    expect(store.batteryDisplay).not.toMatch(/NaN/)
    expect(store.pendingCount).toBe(0)
    expect(store.pendingDisplay).toBe(`0 ${DEVICE_COPY.pendingUnit}`)
    expect(store.weakDeviceHint).toBe(DEVICE_COPY.waitingReport)
  })

  it('reset 作废在途响应', async () => {
    let resolveFetch!: (v: unknown) => void
    getSyncSnapshot.mockReturnValue(
      new Promise((r) => {
        resolveFetch = r
      }),
    )
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    const pending = store.fetchSnapshot({ reason: 'show' })
    store.reset()
    expect(store.snapshot).toBeNull()
    resolveFetch({ code: 0, message: 'ok', data: makeSnapshot() })
    await pending
    expect(store.snapshot).toBeNull()
    expect(store.uiPhase).toBe('loading')
  })

  it('成功且 firmware_version 空 → 弱提示等待设备上报', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ firmware_version: '' }),
    })
    const { useDeviceStatusStore } = await import('./stores/deviceStatus')
    const store = useDeviceStatusStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.weakDeviceHint).toBe(DEVICE_COPY.waitingReport)
  })

  it('正式路径不引用 readingStream；无充电文案；无 uni.request；钉住 onShow/下拉拉数', () => {
    const storeSrc = readFileSync(join(srcRoot, 'stores/deviceStatus.ts'), 'utf8')
    expect(storeSrc).not.toMatch(/from ['"].*readingStream['"]/)
    expect(storeSrc).not.toMatch(/useReadingStreamStore/)
    // 仅检查可执行面：字符串字面量不得含充电文案（注释允许写禁令）
    const storeLiterals = storeSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '')
    expect(storeLiterals).not.toMatch(/['"`][^'"`]*充电/)
    expect(storeLiterals).not.toMatch(/['"`][^'"`]*sync_now/)

    const pageSrc = readFileSync(join(srcRoot, 'pages/device/index.vue'), 'utf8')
    const rowSrc = readFileSync(
      join(srcRoot, 'components/device/DevStatusRow.vue'),
      'utf8',
    )
    const btnSrc = readFileSync(
      join(srcRoot, 'components/device/SyncActionButton.vue'),
      'utf8',
    )
    for (const text of [pageSrc, rowSrc, btnSrc]) {
      expect(text).not.toMatch(/\bTODO\b/)
      expect(text).not.toMatch(/\bdraft\b/i)
      expect(text).not.toMatch(/\bplaceholder\b/i)
      expect(text).not.toMatch(/data-pencil-id/)
      expect(text).not.toMatch(/\buni\.request\b/)
      expect(text).not.toMatch(/未充电/)
      expect(text).not.toMatch(/充电中/)
    }
    expect(pageSrc).toContain('DEVICE_COPY')
    expect(pageSrc).toContain('ensureAuthenticated')
    expect(pageSrc).toContain("setCurrentTab('device')")
    expect(pageSrc).toContain("loadSnapshot('show')")
    expect(pageSrc).toContain('onPullDownRefresh')
    expect(pageSrc).toContain("loadSnapshot('pull')")
    expect(btnSrc).toContain('sync-action--pending')
    expect(btnSrc).toContain('sync-action--ok')
  })

  it('冻结文案槽位对齐 HTML + 产品禁令', () => {
    expect(DEVICE_COPY.pageTitle).toBe('设备')
    expect(DEVICE_COPY.statusConnected).toBe('已连接')
    expect(DEVICE_COPY.statusPending).toBe('待同步')
    expect(DEVICE_COPY.batteryLabel).toBe('电量')
    expect(DEVICE_COPY.networkLabel).toBe('4G')
    expect(DEVICE_COPY.networkConnected).toBe('有信号')
    expect(DEVICE_COPY.networkNoSignal).toBe('无信号')
    expect(DEVICE_COPY.networkDisabled).toBe('已关闭')
    expect(DEVICE_COPY.lastSyncNever).toBe('尚未同步')
    expect(DEVICE_COPY.lastSyncJustNow).toBe('刚刚')
    expect(DEVICE_COPY.commandApplied).toBe('已生效')
    expect(DEVICE_COPY.commandPending).toBe('待设备应用')
    expect(DEVICE_COPY.syncBannerOk).toBe('设备与云端一致')
    expect(DEVICE_COPY.syncBannerPending).toBe('有待同步数据')
    expect(DEVICE_COPY.mainCta).toBe('刷新设备状态')
    expect(DEVICE_COPY.failAction).toBe('重试同步')
  })

  it('api/sync 仅经 request；refreshDeviceSnapshot 别名指向 snapshot', () => {
    const apiSrc = readFileSync(join(srcRoot, 'api/sync.ts'), 'utf8')
    expect(apiSrc).toContain("get<StateSnapshot>('/sync/snapshot'")
    expect(apiSrc).toContain('refreshDeviceSnapshot')
    expect(apiSrc).toContain('return getSyncSnapshot')
    // 可执行面不得出现 sync_now 调用/路径（注释允许写禁令）
    const apiLiterals = apiSrc.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '')
    expect(apiLiterals).not.toMatch(/sync_now/)
    expect(apiSrc).not.toMatch(/\baxios\b/)
    expect(apiSrc).not.toMatch(/\balova\b/i)
    expect(apiSrc).not.toMatch(/\buni\.request\b/)
  })
})
