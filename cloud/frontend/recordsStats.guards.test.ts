/**
 * Story 6.6：记录页统计护栏。
 * ① 字段映射 ② empty→EMPTY ③ pending 不加算 ④ 失败可重试 ⑤ 单飞
 * ⑥ 正式数字不读 readingStream ⑦ 禁用词 / 无第二 uni.request（壳层已扫）
 */
import { readFileSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { createPinia, setActivePinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import { RECORDS_COPY } from './utils/constants'
import type { HistoryStats } from './types/sync'

const getHistoryStats = vi.fn()

vi.mock('./api/sync', () => ({
  getHistoryStats: (...args: unknown[]) => getHistoryStats(...args),
  getSyncSnapshot: vi.fn(),
  postRoundAction: vi.fn(),
}))

const srcRoot = fileURLToPath(new URL('.', import.meta.url))

function makeStats(partial: Partial<HistoryStats> = {}): HistoryStats {
  return {
    today_taps: 128,
    last_7_days_taps: 862,
    last_30_days_taps: 3210,
    total_taps: 3456,
    streak_days: 6,
    empty: false,
    pending_sync: false,
    ...partial,
  }
}

describe('recordsStats store (Story 6.6)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    getHistoryStats.mockReset()
  })

  it('字段映射五指标；千分位格式化', async () => {
    getHistoryStats.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeStats(),
    })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.stats?.today_taps).toBe(128)
    expect(store.stats?.last_7_days_taps).toBe(862)
    expect(store.stats?.last_30_days_taps).toBe(3210)
    expect(store.stats?.total_taps).toBe(3456)
    expect(store.stats?.streak_days).toBe(6)
    expect(store.todayDisplay).toBe('128')
    expect(store.weekDisplay).toBe('862')
    expect(store.monthDisplay).toBe('3,210')
    expect(store.totalDisplay).toBe('3,456')
    expect(store.streakDisplay).toBe('6')
    expect(store.statusCapsule).toBe('confirmed')
  })

  it('empty=true → EMPTY；不把全 0 伪装成有历史', async () => {
    getHistoryStats.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeStats({
        today_taps: 0,
        last_7_days_taps: 0,
        last_30_days_taps: 0,
        total_taps: 0,
        streak_days: 0,
        empty: true,
      }),
    })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('empty')
    expect(RECORDS_COPY.emptyTitle).toBe('暂无记录')
    expect(RECORDS_COPY.emptyHint).toBe('暂无已确认数据')
  })

  it('empty=false 且全 0 仍为 ready（信任响应 empty）', async () => {
    getHistoryStats.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeStats({
        today_taps: 0,
        last_7_days_taps: 0,
        last_30_days_taps: 0,
        total_taps: 0,
        streak_days: 0,
        empty: false,
      }),
    })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.stats?.total_taps).toBe(0)
    expect(store.todayDisplay).toBe('0')
  })

  it('pending_sync 不加算；正式数字仍只绑响应字段', async () => {
    getHistoryStats.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeStats({ today_taps: 10, total_taps: 100, pending_sync: true }),
    })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.stats?.today_taps).toBe(10)
    expect(store.stats?.total_taps).toBe(100)
    expect(store.statusCapsule).toBe('pending')
    expect(RECORDS_COPY.statusPending).toBe('含待同步')
  })

  it('失败可重试；保留上次 ready 数字', async () => {
    getHistoryStats
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeStats({ today_taps: 5 }),
      })
      .mockResolvedValueOnce({
        code: 50000,
        message: '网络错误',
        data: null,
      })
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeStats({ today_taps: 7 }),
      })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.stats?.today_taps).toBe(5)

    await store.fetchStats({ reason: 'retry' })
    expect(store.uiPhase).toBe('fail')
    expect(store.stats?.today_taps).toBe(5)

    await store.fetchStats({ reason: 'retry' })
    expect(store.uiPhase).toBe('ready')
    expect(store.stats?.today_taps).toBe(7)
  })

  it('fail 后再失败仍保留数字，且不闪 loading', async () => {
    getHistoryStats
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeStats({ today_taps: 9 }),
      })
      .mockResolvedValueOnce({
        code: 50000,
        message: '第一次失败',
        data: null,
      })
      .mockResolvedValueOnce({
        code: 50000,
        message: '第二次失败',
        data: null,
      })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')

    await store.fetchStats({ reason: 'retry' })
    expect(store.uiPhase).toBe('fail')
    expect(store.stats?.today_taps).toBe(9)

    const phaseDuringRetry = store.uiPhase
    const retryPromise = store.fetchStats({ reason: 'retry' })
    // 已有 stats 时不得先切 loading
    expect(store.uiPhase).toBe(phaseDuringRetry)
    expect(store.uiPhase).not.toBe('loading')
    expect(store.stats?.today_taps).toBe(9)
    await retryPromise
    expect(store.uiPhase).toBe('fail')
    expect(store.stats?.today_taps).toBe(9)
  })

  it('首次失败 → 纯 fail 且 stats 为空', async () => {
    getHistoryStats.mockResolvedValue({
      code: 50000,
      message: '网络错误',
      data: null,
    })
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    await store.fetchStats({ reason: 'show' })
    expect(store.uiPhase).toBe('fail')
    expect(store.stats).toBeNull()
  })

  it('reset 作废在途响应，不写回旧会话统计', async () => {
    let resolveFetch!: (v: unknown) => void
    getHistoryStats.mockReturnValue(
      new Promise((r) => {
        resolveFetch = r
      }),
    )
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    const pending = store.fetchStats({ reason: 'show' })
    store.reset()
    expect(store.stats).toBeNull()
    expect(store.uiPhase).toBe('loading')
    resolveFetch({ code: 0, message: 'ok', data: makeStats({ today_taps: 99 }) })
    await pending
    expect(store.stats).toBeNull()
    expect(store.uiPhase).toBe('loading')
  })

  it('刷新单飞：并发 fetch 只发一次请求', async () => {
    let resolve!: (v: unknown) => void
    const pending = new Promise((r) => {
      resolve = r
    })
    getHistoryStats.mockReturnValue(pending)
    const { useRecordsStatsStore } = await import('./stores/recordsStats')
    const store = useRecordsStatsStore()
    const a = store.fetchStats({ reason: 'show' })
    const b = store.fetchStats({ reason: 'pull' })
    const c = store.fetchStats({ reason: 'retry' })
    expect(getHistoryStats).toHaveBeenCalledTimes(1)
    resolve({ code: 0, message: 'ok', data: makeStats() })
    await Promise.all([a, b, c])
    expect(getHistoryStats).toHaveBeenCalledTimes(1)
    expect(store.uiPhase).toBe('ready')
  })

  it('正式数字路径不引用 readingStream / storage', async () => {
    const storeSrc = readFileSync(join(srcRoot, 'stores/recordsStats.ts'), 'utf8')
    // 仅检查 import/调用面；注释中允许出现「禁止 readingStream」说明
    expect(storeSrc).not.toMatch(/from ['"].*readingStream['"]/)
    expect(storeSrc).not.toMatch(/useReadingStreamStore/)
    expect(storeSrc).not.toMatch(/from ['"]\.\.\/utils\/storage['"]/)
    expect(storeSrc).not.toMatch(/\backed_total\b/)
    const pageSrc = readFileSync(join(srcRoot, 'pages/records/index.vue'), 'utf8')
    expect(pageSrc).not.toMatch(/from ['"].*readingStream['"]/)
    expect(pageSrc).not.toMatch(/useReadingStreamStore/)
    expect(pageSrc).not.toMatch(/尚无确认记录/)
    expect(pageSrc).toContain('RECORDS_COPY')
    expect(pageSrc).toMatch(/uiPhase === 'empty'/)
  })

  it('records 页无禁用词；无 uni.request', () => {
    const pageSrc = readFileSync(join(srcRoot, 'pages/records/index.vue'), 'utf8')
    const cardSrc = readFileSync(join(srcRoot, 'components/records/StatCard.vue'), 'utf8')
    const ledgerSrc = readFileSync(
      join(srcRoot, 'components/records/StatLedgerRow.vue'),
      'utf8',
    )
    for (const text of [pageSrc, cardSrc, ledgerSrc]) {
      expect(text).not.toMatch(/\bTODO\b/)
      expect(text).not.toMatch(/\bdraft\b/i)
      expect(text).not.toMatch(/\bplaceholder\b/i)
      expect(text).not.toMatch(/data-pencil-id/)
      expect(text).not.toMatch(/\buni\.request\b/)
    }
  })

  it('冻结文案槽位对齐 HTML', () => {
    expect(RECORDS_COPY.todayLabel).toBe('今日敲击')
    expect(RECORDS_COPY.weekLabel).toBe('近 7 日')
    expect(RECORDS_COPY.monthLabel).toBe('近 30 日')
    expect(RECORDS_COPY.totalLabel).toBe('累计敲击')
    expect(RECORDS_COPY.streakLabel).toBe('连续诵读')
    expect(RECORDS_COPY.failTitle).toBe('同步失败')
    expect(RECORDS_COPY.failAction).toBe('重试同步')
    expect(RECORDS_COPY.statusConfirmed).toBe('已确认数据')
    expect(RECORDS_COPY.refreshedToast).toBe('已刷新')
  })
})
