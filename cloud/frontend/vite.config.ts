/**
 * Story 6.1 裁决 A：以官方 @dcloudio/vite-plugin-uni 在现有 cloud/frontend 演进；
 * 仅交付 mp-weixin 脚本，不引入 H5/App 业务分支。
 */
import { defineConfig } from 'vite'
import uni from '@dcloudio/vite-plugin-uni'

export default defineConfig({
  plugins: [uni()],
})
