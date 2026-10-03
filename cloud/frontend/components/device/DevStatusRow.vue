<template>
  <view class="dev-row" :class="{ 'dev-row--summary': variant === 'summary' }">
    <template v-if="variant === 'summary'">
      <view class="dev-row__cell">
        <view class="dev-row__label-row">
          <view class="dev-row__icon dev-row__icon--battery" aria-hidden="true">
            <view class="dev-row__icon-battery-body" />
            <view class="dev-row__icon-battery-cap" />
          </view>
          <text class="dev-row__label">{{ leftLabel }}</text>
        </view>
        <text class="dev-row__value">{{ leftValue }}</text>
      </view>
      <view class="dev-row__v-rule" />
      <view class="dev-row__cell">
        <view class="dev-row__label-row">
          <view
            class="dev-row__icon dev-row__icon--net"
            :class="`dev-row__icon--net-${netTone}`"
            aria-hidden="true"
          >
            <view class="dev-row__icon-bar" style="height: 6px" />
            <view class="dev-row__icon-bar" style="height: 10px" />
            <view class="dev-row__icon-bar" style="height: 14px" />
          </view>
          <text class="dev-row__label">{{ rightLabel }}</text>
        </view>
        <text class="dev-row__value">{{ rightValue }}</text>
      </view>
    </template>
    <template v-else>
      <view class="dev-row__label-row">
        <view
          v-if="icon"
          class="dev-row__icon-mark"
          aria-hidden="true"
        >
          <text class="dev-row__icon-mark-text">{{ icon }}</text>
        </view>
        <text class="dev-row__label">{{ label }}</text>
      </view>
      <text
        class="dev-row__value dev-row__value--row"
        :class="{ 'dev-row__value--accent': accent }"
      >{{ value }}</text>
    </template>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.7：devstatus-row——摘要卡（电量+4G）或账本行。
 * Story 6.9：图标+文字双编码（令牌色 CSS 形）；禁止装饰背景冒充数据、禁止充电文案。
 * 网络信号柱随 networkMode 变化，避免「无信号/已关闭」仍满格。
 */
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    variant?: 'summary' | 'row'
    label?: string
    value?: string
    accent?: boolean
    /** 行级可选字符图标（双通道）；摘要卡用电量/信号形）。 */
    icon?: string
    /** connected | no_signal | disabled —— 摘要卡网络图标态。 */
    networkMode?: 'connected' | 'no_signal' | 'disabled'
    leftLabel?: string
    leftValue?: string
    rightLabel?: string
    rightValue?: string
  }>(),
  { variant: 'row', accent: false, networkMode: 'no_signal' },
)

const netTone = computed(() => props.networkMode ?? 'no_signal')
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.dev-row--summary {
  display: flex;
  flex-direction: row;
  align-items: stretch;
  box-sizing: border-box;
  min-height: 88px;
  padding: 14px 16px;
  background-color: $card;
  outline: 1px solid $divider;
  outline-offset: -0.5px;
  border-radius: 16px;
}

.dev-row__cell {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6px;
  box-sizing: border-box;
}

.dev-row__label-row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
}

.dev-row__icon {
  flex-shrink: 0;
  display: flex;
  align-items: flex-end;
  justify-content: center;
}

.dev-row__icon--battery {
  width: 16px;
  height: 12px;
  position: relative;
}

.dev-row__icon-battery-body {
  width: 12px;
  height: 10px;
  border: 1.5px solid $accent;
  border-radius: 2px;
  box-sizing: border-box;
}

.dev-row__icon-battery-cap {
  position: absolute;
  right: 0;
  top: 3px;
  width: 2px;
  height: 4px;
  background-color: $accent;
  border-radius: 0 1px 1px 0;
}

.dev-row__icon--net {
  width: 16px;
  height: 14px;
  gap: 2px;
  flex-direction: row;
}

.dev-row__icon-bar {
  width: 3px;
  background-color: $accent;
  border-radius: 1px;
  align-self: flex-end;
}

.dev-row__icon--net-no_signal .dev-row__icon-bar:nth-child(2),
.dev-row__icon--net-no_signal .dev-row__icon-bar:nth-child(3) {
  opacity: 0.22;
}

.dev-row__icon--net-disabled .dev-row__icon-bar {
  opacity: 0.28;
  background-color: $ink-2;
}

.dev-row__icon-mark {
  width: 16px;
  height: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.dev-row__icon-mark-text {
  color: $accent;
  font-size: 11px;
  font-weight: 700;
  line-height: 1;
}

.dev-row__v-rule {
  width: 1px;
  align-self: stretch;
  background-color: $divider;
  margin: 0 12px;
  flex-shrink: 0;
}

.dev-row:not(.dev-row--summary) {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  min-height: 56px;
  padding: 12px 0;
  box-sizing: border-box;
}

.dev-row__label {
  color: $ink-2;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.4;
}

.dev-row--summary .dev-row__label {
  font-size: 13px;
}

.dev-row__value {
  color: $ink;
  font-size: 16px;
  font-weight: 600;
  line-height: 1.2;
}

.dev-row__value--row {
  font-size: 14px;
  text-align: left;
  max-width: 50%;
}

.dev-row__value--accent {
  color: $accent;
}
</style>
