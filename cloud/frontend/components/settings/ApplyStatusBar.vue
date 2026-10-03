<template>
  <view
    class="apply-bar"
    :class="{
      'apply-bar--pending': status === 'pending',
      'apply-bar--applied': status === 'applied',
    }"
  >
    <view class="apply-bar__icon" aria-hidden="true">
      <text class="apply-bar__icon-text">{{ status === 'pending' ? '待' : '效' }}</text>
    </view>
    <text class="apply-bar__text">{{ label }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.8：待设备应用 / 已生效状态条。
 * Story 6.9：图标+文字双通道（非纯色传达）。
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
  gap: 8px;
  background-color: $fill-muted;
  border-radius: 12px;
}

.apply-bar__icon {
  width: 22px;
  height: 22px;
  border-radius: 6px;
  background-color: rgba(166, 107, 58, 0.18);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.apply-bar__icon-text {
  color: $accent;
  font-size: 11px;
  font-weight: 700;
  line-height: 1;
}

.apply-bar__text {
  color: $accent;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}
</style>
