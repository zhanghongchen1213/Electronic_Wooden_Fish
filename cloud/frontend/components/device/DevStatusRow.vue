<template>
  <view class="dev-row" :class="{ 'dev-row--summary': variant === 'summary' }">
    <template v-if="variant === 'summary'">
      <view class="dev-row__cell">
        <view class="dev-row__label-row">
          <view
            class="dev-row__icon"
            aria-hidden="true"
            :style="{ backgroundImage: `url(${iconDataUri('battery', '#8b8177')})` }"
          />
          <text class="dev-row__label">{{ leftLabel }}</text>
        </view>
        <text class="dev-row__value">{{ leftValue }}</text>
      </view>
      <view class="dev-row__v-rule" />
      <view class="dev-row__cell">
        <view class="dev-row__label-row">
          <view
            class="dev-row__icon"
            :class="`dev-row__icon--${netTone}`"
            aria-hidden="true"
            :style="{ backgroundImage: `url(${iconDataUri('radio', '#8b8177')})` }"
          />
          <text class="dev-row__label">{{ rightLabel }}</text>
        </view>
        <text class="dev-row__value">{{ rightValue }}</text>
      </view>
    </template>
    <template v-else>
      <view class="dev-row__label-row">
        <view
          v-if="icon"
          class="dev-row__icon dev-row__icon--row"
          aria-hidden="true"
          :style="{ backgroundImage: `url(${iconDataUri(icon, '#8b8177')})` }"
        />
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
 * 2026-10-03 对拍：图标对齐导出稿 lucide battery/radio/refresh-cw/upload/settings；
 * 网络图标随 networkMode 弱化（透明度通道），避免「无信号/已关闭」仍满格观感。
 * 禁止装饰背景冒充数据、禁止充电文案。
 */
import { computed } from 'vue'
import { iconDataUri, type IconKey } from '../../utils/uiIcons'

const props = withDefaults(
  defineProps<{
    variant?: 'summary' | 'row'
    label?: string
    value?: string
    accent?: boolean
    /** 行级可选图标（设计稿 lucide 键名）。 */
    icon?: IconKey
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
  outline: 1px solid $rule-weak;
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
  gap: 12px;
}

.dev-row__icon {
  width: 18px;
  height: 18px;
  background-repeat: no-repeat;
  background-size: 100% 100%;
  flex-shrink: 0;
}

.dev-row__icon--no_signal {
  opacity: 0.4;
}

.dev-row__icon--disabled {
  opacity: 0.4;
  filter: grayscale(1);
}

.dev-row__icon--row {
  width: 18px;
  height: 18px;
}

.dev-row__v-rule {
  width: 1px;
  align-self: stretch;
  background-color: $rule-weak;
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
  font-family: $font-sans;
  font-size: 14px;
  font-weight: 600;
  line-height: 17px;
}

.dev-row--summary .dev-row__label {
  font-size: 13px;
  line-height: 16px;
}

.dev-row__value {
  color: $ink;
  font-family: $font-sans;
  font-size: 16px;
  font-weight: 600;
  line-height: 19px;
}

.dev-row__value--row {
  font-size: 14px;
  line-height: 17px;
  text-align: left;
  max-width: 50%;
}

.dev-row__value--accent {
  color: $accent;
}
</style>
