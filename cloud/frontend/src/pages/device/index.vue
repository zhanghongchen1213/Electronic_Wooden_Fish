<template>
  <page-shell title="设备">
    <view class="empty">
      <text class="empty__text">等待设备状态</text>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：设备一级页壳。设备状态与同步动作属 6.7。
 */
import { onShow } from '@dcloudio/uni-app'
import PageShell from '../../components/page-shell/PageShell.vue'
import { useAppShellStore } from '../../stores/appShell'

const shell = useAppShellStore()
onShow(() => {
  shell.setCurrentTab('device')
})
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.empty {
  padding-top: 48px;
}

.empty__text {
  color: $ink-3;
  font-size: 14px;
  line-height: 1.6;
}
</style>
