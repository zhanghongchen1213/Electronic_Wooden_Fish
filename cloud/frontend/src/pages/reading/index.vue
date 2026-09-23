<template>
  <page-shell title="阅读">
    <view class="empty">
      <text class="empty__text">等待设备诵读</text>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：阅读一级页壳。阅读流属 6.3；禁止可点击木鱼与假进度。
 * 裁决 B：一级切换由原生 tabBar 承担。
 */
import { onShow } from '@dcloudio/uni-app'
import PageShell from '../../components/page-shell/PageShell.vue'
import { useAppShellStore } from '../../stores/appShell'

const shell = useAppShellStore()
onShow(() => {
  shell.setCurrentTab('reading')
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
