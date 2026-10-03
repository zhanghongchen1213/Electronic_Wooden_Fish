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
 * 2026-10-03 对拍：数值衬线体——双列 30px/36px、行 28px/34px（含单位同尺寸）；
 * 行首字符图标不再被页面传参（设计稿账本行无图标），组件能力保留。
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
  background-color: $rule-weak;
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
  font-family: $font-sans;
  font-size: 14px;
  font-weight: 600;
  line-height: 17px;
}

.ledger--pair .ledger__label {
  font-size: 13px;
  line-height: 16px;
}

.ledger__value {
  color: $ink;
  font-family: $font-serif;
  font-size: 28px;
  font-weight: 700;
  line-height: 34px;
}

.ledger--pair .ledger__value {
  font-size: 30px;
  line-height: 36px;
}

.ledger__value--md {
  margin-top: 8px;
}

.ledger__value--row {
  text-align: left;
  max-width: 50%;
}

.ledger__unit {
  color: $ink;
  font-size: inherit;
  font-weight: inherit;
}
</style>
