<template>
  <view class="ledger" :class="{ 'ledger--pair': variant === 'pair' }">
    <template v-if="variant === 'pair'">
      <view class="ledger__col">
        <view class="ledger__label-row">
          <view
            v-if="leftIcon"
            class="ledger__icon"
            aria-hidden="true"
          >
            <text class="ledger__icon-text">{{ leftIcon }}</text>
          </view>
          <text class="ledger__label">{{ leftLabel }}</text>
        </view>
        <text class="ledger__value ledger__value--md">{{ leftValue }}</text>
      </view>
      <view class="ledger__v-rule" />
      <view class="ledger__col">
        <view class="ledger__label-row">
          <view
            v-if="rightIcon"
            class="ledger__icon"
            aria-hidden="true"
          >
            <text class="ledger__icon-text">{{ rightIcon }}</text>
          </view>
          <text class="ledger__label">{{ rightLabel }}</text>
        </view>
        <text class="ledger__value ledger__value--md">{{ rightValue }}</text>
      </view>
    </template>
    <template v-else>
      <view class="ledger__label-row">
        <view
          v-if="icon"
          class="ledger__icon"
          aria-hidden="true"
        >
          <text class="ledger__icon-text">{{ icon }}</text>
        </view>
        <text class="ledger__label">{{ label }}</text>
      </view>
      <text class="ledger__value ledger__value--row">
        {{ value }}<text v-if="unit" class="ledger__unit"> {{ unit }}</text>
      </text>
    </template>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.6：账本行 / 近 7·30 双列（无盒化；仅低对比刻线分隔）。
 * Story 6.9：可选字符图标双编码（Task 3.2 记录行）。
 */
withDefaults(
  defineProps<{
    variant?: 'pair' | 'row'
    label?: string
    value?: string
    unit?: string
    icon?: string
    leftLabel?: string
    leftValue?: string
    leftIcon?: string
    rightLabel?: string
    rightValue?: string
    rightIcon?: string
  }>(),
  { variant: 'row' },
)
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.ledger {
  box-sizing: border-box;
}

.ledger--pair {
  display: flex;
  flex-direction: row;
  align-items: stretch;
  min-height: 92px;
}

.ledger__col {
  flex: 1;
  padding: 10px 0;
  box-sizing: border-box;
}

.ledger__label-row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
}

.ledger__icon {
  width: 16px;
  height: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.ledger__icon-text {
  color: $accent;
  font-size: 11px;
  font-weight: 700;
  line-height: 1;
}

.ledger__v-rule {
  width: 1px;
  align-self: stretch;
  background-color: $divider;
  margin: 0 8px;
  flex-shrink: 0;
}

.ledger:not(.ledger--pair) {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  min-height: 56px;
  padding: 12px 0;
}

.ledger__label {
  color: $ink-2;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.4;
}

.ledger__value {
  color: $ink;
  font-size: 16px;
  font-weight: 600;
  line-height: 1.2;
}

.ledger__value--md {
  margin-top: 6px;
  font-size: 18px;
}

.ledger__value--row {
  font-size: 14px;
  text-align: left;
  max-width: 50%;
}

.ledger__unit {
  color: $ink-2;
  font-size: 12px;
  font-weight: 500;
}
</style>
