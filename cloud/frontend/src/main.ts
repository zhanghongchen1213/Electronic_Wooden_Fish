/**
 * Story 6.1 裁决 A/D/F：uni-app Vue3 入口。
 * - 复用 5.6 request/storage，启动绑定真 uni；不引入 Alova/第二 HTTP 栈。
 * - 登录直达与 wx.login 属 6.2；本文件只接线壳层。
 */
import { createSSRApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import { bindUniAdapter } from './api/request'
import { assertNoSessionKeyStorage, bindUniStorage } from './utils/storage'

export function createApp() {
  const app = createSSRApp(App)
  const pinia = createPinia()
  app.use(pinia)

  // 生产/开发者工具：绑定真实 uni；Node vitest 仍走测试注入替身。
  const g = globalThis as {
    uni?: Parameters<typeof bindUniAdapter>[0] & Parameters<typeof bindUniStorage>[0]
  }
  if (g.uni) {
    bindUniAdapter(g.uni)
    bindUniStorage(g.uni)
    assertNoSessionKeyStorage()
  }

  return { app }
}
