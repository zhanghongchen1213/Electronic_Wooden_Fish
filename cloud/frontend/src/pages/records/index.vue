<template>
  <page-shell title="记录">
    <view class="empty">
      <text class="empty__text">尚无确认记录</text>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：记录一级页壳。统计业务属 6.6；禁止伪造数字。
 */
import { onShow } from '@dcloudio/uni-app'
import PageShell from '../../components/page-shell/PageShell.vue'
import { useAppShellStore } from '../../stores/appShell'

const shell = useAppShellStore()
onShow(() => {
  shell.setCurrentTab('records')
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
