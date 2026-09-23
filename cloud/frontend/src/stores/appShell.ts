/**
 * Story 6.1 裁决 D + AD-2：壳层非权威 UI 状态。
 *
 * 本 store 只记录当前一级页等本地 UI 标志，不得作为 cloud 权威进度、
 * 统计或命令状态的同步基准；权威数据只来自 backend。
 */
import { defineStore } from 'pinia'
import type { PrimaryTabKey } from '../utils/constants'

export const useAppShellStore = defineStore('appShell', {
  state: () => ({
    /** 非权威：当前一级页键，仅供导航高亮与壳层路由。 */
    currentTab: 'reading' as PrimaryTabKey,
    /** 非权威：应用是否已 onLaunch；不作业务就绪判定。 */
    launched: false,
  }),
  actions: {
    setCurrentTab(tab: PrimaryTabKey) {
      this.currentTab = tab
    },
    markLaunched() {
      this.launched = true
    },
  },
})
