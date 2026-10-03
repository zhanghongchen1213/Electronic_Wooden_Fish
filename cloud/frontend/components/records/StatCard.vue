<template>
  <view class="stat-card">
    <text class="stat-card__label">{{ label }}</text>
    <view class="stat-card__value-row">
      <text class="stat-card__value">{{ value }}</text>
      <text v-if="unit" class="stat-card__unit">{{ unit }}</text>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.6：今日大数卡（语义必要填充面；非仪表盘卡片墙）。
 * 令牌：card #fffdf6、低对比刻线 outline；禁止装饰渐变伪造层级。
 */
defineProps<{
  label: string
  value: string
  unit?: string
}>()
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.stat-card {
  background-color: $card;
  outline: 1px solid $divider;
  outline-offset: -0.5px;
  border-radius: 16px;
  padding: 14px 16px 20px;
  box-sizing: border-box;
}

.stat-card__label {
  display: block;
  color: $ink-2;
  font-size: 13px;
  font-weight: 600;
  line-height: 1.4;
  margin-bottom: 8px;
}

.stat-card__value-row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 2px;
}

.stat-card__value {
  color: $ink;
  font-size: 56px;
  font-weight: 700;
  line-height: 1.1;
}

.stat-card__unit {
  color: $ink-2;
  font-size: 16px;
  font-weight: 600;
  line-height: 1.2;
  margin-top: 18px;
}
</style>
