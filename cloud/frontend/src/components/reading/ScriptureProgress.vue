<template>
  <view class="scripture-progress">
    <text class="scripture-progress__label">{{ label }}</text>
    <view class="scripture-progress__row">
      <text class="scripture-progress__count">{{ view.countLabel }}</text>
      <text class="scripture-progress__percent">{{ view.percentLabel }}</text>
    </view>
    <view class="scripture-progress__track">
      <view
        class="scripture-progress__value"
        :style="{ width: `${view.percent}%` }"
      />
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.3 裁决 E：唯一 scripture-progress。
 * 计数 `${cursor} / ${260} 字`；百分比 1 位小数；轨道填充 focus 色。
 * 禁止第二进度条。
 */
import { computed } from 'vue'
import { READING_COPY } from '../../utils/constants'
import { progressView } from '../../utils/scriptureProjection'

const props = defineProps<{
  cursor: number
}>()

const label = READING_COPY.progressLabel
const view = computed(() => progressView(props.cursor))
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.scripture-progress {
  background-color: $card;
  border: 1px solid $divider;
  border-radius: 16px;
  padding: 12px 16px 16px;
  box-sizing: border-box;
}

.scripture-progress__label {
  font-size: 13px;
  line-height: 16px;
  color: $ink-2;
  font-weight: 600;
}

.scripture-progress__row {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  margin-top: 6px;
}

.scripture-progress__count {
  font-size: 18px;
  line-height: 22px;
  color: $ink;
  font-weight: 600;
}

.scripture-progress__percent {
  font-size: 18px;
  line-height: 22px;
  color: $accent;
  font-weight: 700;
}

.scripture-progress__track {
  margin-top: 14px;
  height: 5px;
  background-color: $fill-muted;
  border-radius: 3px;
  overflow: hidden;
}

.scripture-progress__value {
  height: 5px;
  background-color: $accent;
  border-radius: 3px;
  max-width: 100%;
}
</style>
