<template>
  <page-shell title="设置">
    <view class="empty">
      <text class="empty__text">设置将由设备确认后生效</text>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：设置一级页壳。镜像与待应用属 6.8。
 */
import { onShow } from '@dcloudio/uni-app'
import PageShell from '../../components/page-shell/PageShell.vue'
import { useAppShellStore } from '../../stores/appShell'

const shell = useAppShellStore()
onShow(() => {
  shell.setCurrentTab('settings')
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
