<template>
  <view class="scripture-progress">
    <text class="scripture-progress__label">{{ label }}</text>
    <view class="scripture-progress__row">
      <text class="scripture-progress__count">{{ view.countLabel }}</text>
      <text class="scripture-progress__percent">{{ view.percentLabel }}</text>
    </view>
    <view class="scripture-progress__gauge">
      <view
        v-for="n in 7"
        :key="n"
        class="scripture-progress__tick"
      />
      <view class="scripture-progress__track">
        <view
          class="scripture-progress__value"
          :style="{ width: `${view.percent}%` }"
        />
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.3 裁决 E：唯一 scripture-progress。
 * 计数 `${cursor} / ${260} 字`；百分比 1 位小数；轨道填充 focus 色。
 * 2026-10-03 对拍：7 根刻度（1×16、间隔 53px）为导出稿 tick-0..6 真源，
 * 非 Story 6.9 所禁的自造 17 刻度；卡片描边对齐 outline 手法。
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
  outline: 1px solid $rule-weak;
  outline-offset: -0.5px;
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

.scripture-progress__gauge {
  position: relative;
  margin-top: 14px;
  height: 16px;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.scripture-progress__tick {
  position: absolute;
  top: 0;
  width: 1px;
  height: 16px;
  background-color: $rule-weak;
}

/* 7 根刻度均布于 318px 轨道（首尾各一，间隔 53px），压在轨道上方 */
.scripture-progress__tick:nth-child(1) { left: 0; }
.scripture-progress__tick:nth-child(2) { left: 53px; }
.scripture-progress__tick:nth-child(3) { left: 106px; }
.scripture-progress__tick:nth-child(4) { left: 159px; }
.scripture-progress__tick:nth-child(5) { left: 212px; }
.scripture-progress__tick:nth-child(6) { left: 265px; }
.scripture-progress__tick:nth-child(7) { left: 317px; }

.scripture-progress__track {
  height: 5px;
  width: 100%;
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
