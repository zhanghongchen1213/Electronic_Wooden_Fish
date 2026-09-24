<script setup lang="ts">
/**
 * Story 6.1 裁决 C：纸面根帧 = 全 app 唯一大底（#f4f0e5）。
 * Story 6.2 裁决 B：onLaunch 统一冷启动令牌门闸；未登录走 redirectToLogin 判重。
 * WebSocket / 业务拉取属后续 Story；门闸不假连。
 */
import { onLaunch } from '@dcloudio/uni-app'
import { useAppShellStore } from './stores/appShell'
import { ensureAuthenticated, isOnLoginPage } from './utils/authGate'

onLaunch(() => {
  const shell = useAppShellStore()
  shell.markLaunched()
  if (!isOnLoginPage()) {
    ensureAuthenticated()
  }
})
</script>

<style lang="scss">
@import './styles/tokens.scss';

page {
  background-color: $bg;
  color: $ink;
  min-height: 100%;
  /* 画布对拍基线：390×844；真机等比缩放，语义不变 */
  box-sizing: border-box;
}
</style>
