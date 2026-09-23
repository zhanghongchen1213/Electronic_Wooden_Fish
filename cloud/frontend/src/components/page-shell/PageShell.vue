<template>
  <view class="page-shell">
    <view class="page-header">
      <text class="page-header__title">{{ title }}</text>
    </view>
    <view class="page-body">
      <slot />
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.1 裁决 C：page-header 自 y=62 起；左右 page_pad=20；无业务假数据。
 */
defineProps<{
  title: string
}>()
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.page-shell {
  min-height: 100vh;
  background-color: $bg;
  box-sizing: border-box;
  padding-bottom: $nav-h;
}

.page-header {
  padding-top: $safe-top;
  padding-left: $page-pad;
  padding-right: $page-pad;
  padding-bottom: 12px;
}

.page-header__title {
  font-size: 18px;
  font-weight: 600;
  color: $ink;
  line-height: 1.4;
}

.page-body {
  padding-left: $page-pad;
  padding-right: $page-pad;
}
</style>
