<script setup lang="ts">
/**
 * Story 6.1 裁决 C：纸面根帧 = 全 app 唯一大底（#f4f0e5）。
 * 2026-10-02 裁决：完全自用测试，移除冷启动鉴权门闸。
 */
import { onLaunch } from '@dcloudio/uni-app'
import { useAppShellStore } from './stores/appShell'

onLaunch(() => {
  const shell = useAppShellStore()
  shell.markLaunched()
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
