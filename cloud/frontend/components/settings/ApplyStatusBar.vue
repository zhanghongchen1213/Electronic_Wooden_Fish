<template>
  <view
    class="apply-bar"
    :class="{
      'apply-bar--pending': status === 'pending',
      'apply-bar--applied': status === 'applied',
    }"
  >
    <text class="apply-bar__text">{{ label }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.8：待设备应用 / 已生效状态条。
 * 2026-10-03 对拍：纯居中文字横幅（对拍导出稿 setting-mirror 状态横幅，
 * 无图标块）；文字本身即状态双通道（「待设备应用」/「已生效」）。
 * 文案由修订对派生，不是 wire 字段（裁决与 AD-5）。
 */
defineProps<{
  status: 'applied' | 'pending'
  label: string
}>()
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.apply-bar {
  box-sizing: border-box;
  width: 100%;
  min-height: 48px;
  padding: 12px 16px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: $fill-muted;
  border-radius: 12px;
}

.apply-bar__text {
  color: $accent;
  font-family: $font-sans;
  font-size: 14px;
  font-weight: 600;
  line-height: 17px;
  text-align: center;
}
</style>
