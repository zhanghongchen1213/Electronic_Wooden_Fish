/**
 * Story 6.6：记录页统计展示缓存（非权威）。
 *
 * 裁决 A：仅 GET /sync/stats；不用 snapshot / WS / readingStream 拼数字。
 * 裁决 B：完全信任响应 empty；前端不自行 total===0 重判。
 * 裁决 C：pending_sync 不加算；页眉弱提示，正式五数字只绑响应字段。
 * 裁决 D：失败时保留上次 ready 数字并叠加 fail；首次失败则纯 fail。
 * 裁决 E：inflight Promise 单飞；下拉与重试共用。
 * 裁决 F：整数原样 + 轻量千分位；单位「次」/「天」。
 * 裁决 G：不做设备/印谱像素终验（6.7/6.9）。
 *
 * 禁止从 readingStream / storage 推导正式数字（AD-2 / AD-6）。
 */
import { defineStore } from 'pinia'
import { getHistoryStats } from '../api/sync'
import { normalizeToastErrorMessage } from '../api/requestMessage'
import { SUCCESS_CODE } from '../utils/constants'
import type { HistoryStats } from '../types/sync'

export type RecordsUiPhase = 'loading' | 'empty' | 'ready' | 'fail'

export type FetchStatsReason = 'show' | 'pull' | 'retry'

function formatTapCount(n: number): string {
  return new Intl.NumberFormat('en-US').format(Math.trunc(n))
}

export const useRecordsStatsStore = defineStore('recordsStats', {
  state: () => ({
    uiPhase: 'loading' as RecordsUiPhase,
    stats: null as HistoryStats | null,
    lastError: '' as string,
    refreshing: false,
    /** 单飞锁：下拉与重试共享同一 Promise（裁决 E）。 */
    _inflight: null as Promise<void> | null,
    /** 登出/reset 后作废在途响应，避免写回上一会话。 */
    _fetchGen: 0,
  }),
  getters: {
    /** 页眉胶囊：ready 且 pending_sync →「含待同步」；否则「已确认数据」。 */
    statusCapsule(state): 'confirmed' | 'pending' | 'none' {
      if (state.uiPhase !== 'ready' && state.uiPhase !== 'fail') {
        return 'none'
      }
      if (!state.stats) {
        return 'none'
      }
      return state.stats.pending_sync ? 'pending' : 'confirmed'
    },
    todayDisplay(state): string {
      return state.stats ? formatTapCount(state.stats.today_taps) : ''
    },
    weekDisplay(state): string {
      return state.stats ? formatTapCount(state.stats.last_7_days_taps) : ''
    },
    monthDisplay(state): string {
      return state.stats ? formatTapCount(state.stats.last_30_days_taps) : ''
    },
    totalDisplay(state): string {
      return state.stats ? formatTapCount(state.stats.total_taps) : ''
    },
    streakDisplay(state): string {
      return state.stats ? formatTapCount(state.stats.streak_days) : ''
    },
  },
  actions: {
    reset(): void {
      this._fetchGen += 1
      this.uiPhase = 'loading'
      this.stats = null
      this.lastError = ''
      this.refreshing = false
      this._inflight = null
    },

    /**
     * 拉取统计。reason=pull 时置 refreshing；完成由调用方 stopPullDownRefresh。
     */
    async fetchStats(opts: { reason: FetchStatsReason } = { reason: 'show' }): Promise<void> {
      if (this._inflight) {
        return this._inflight
      }

      // 裁决 D：以「是否已有成功数字」判定保留，而非当前 uiPhase===ready。
      // fail 叠加态再重试时仍保留数字，避免闪 loading / 二次失败清数。
      const retainStats = this.stats !== null
      if (opts.reason === 'pull') {
        this.refreshing = true
      } else if (!retainStats) {
        this.uiPhase = 'loading'
      }
      this.lastError = ''

      const gen = this._fetchGen
      const pending = this._doFetch(retainStats, gen)
      this._inflight = pending
      try {
        await pending
      } finally {
        if (this._inflight === pending) {
          this._inflight = null
        }
        if (gen === this._fetchGen) {
          this.refreshing = false
        }
      }
    },

    async _doFetch(retainStats: boolean, gen: number): Promise<void> {
      try {
        const body = await getHistoryStats()
        if (gen !== this._fetchGen) {
          return
        }
        if (body.code !== SUCCESS_CODE || !body.data) {
          this.lastError = normalizeToastErrorMessage(body.message) || '同步失败'
          this.uiPhase = 'fail'
          if (!retainStats) {
            this.stats = null
          }
          return
        }
        const data = body.data
        // 裁决 B：完全信任 empty；不自行用全 0 重判
        this.stats = {
          today_taps: data.today_taps,
          last_7_days_taps: data.last_7_days_taps,
          last_30_days_taps: data.last_30_days_taps,
          total_taps: data.total_taps,
          streak_days: data.streak_days,
          empty: data.empty,
          pending_sync: data.pending_sync,
        }
        this.lastError = ''
        this.uiPhase = data.empty ? 'empty' : 'ready'
      } catch (err) {
        if (gen !== this._fetchGen) {
          return
        }
        const msg = err instanceof Error ? err.message : String(err)
        this.lastError = normalizeToastErrorMessage(msg) || '同步失败'
        this.uiPhase = 'fail'
        if (!retainStats) {
          this.stats = null
        }
      }
    },
  },
})
