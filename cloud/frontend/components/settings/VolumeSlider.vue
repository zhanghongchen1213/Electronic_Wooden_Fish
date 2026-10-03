<template>
  <view class="volume">
    <view class="volume__head">
      <view
        v-if="icon"
        class="volume__icon"
        aria-hidden="true"
        :style="{ backgroundImage: `url(${iconDataUri(icon, '#8b8177')})` }"
      />
      <text class="volume__label">{{ label }}</text>
    </view>
    <slider
      class="volume__slider"
      :value="modelValue"
      :min="0"
      :max="100"
      :step="1"
      :disabled="disabled"
      :activeColor="activeColor"
      :backgroundColor="trackColor"
      :block-size="24"
      @change="onChange"
    />
    <text class="volume__value">{{ modelValue }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.8：音量滑杆。仅改本地编辑值；不在 changing 过程中打 API（裁决 B）。
 * 2026-10-03 对拍：单行布局（图标+label 左、滑杆居中、数值右）；
 * 数值 15px/18px 700 墨色；knob 24。触区由行高与 slider 保证 ≥44。
 */
import { iconDataUri, type IconKey } from '../../utils/uiIcons'

const props = withDefaults(
  defineProps<{
    label: string
    modelValue: number
    disabled?: boolean
    /** 行首图标（设计稿 lucide 键名）。 */
    icon?: IconKey
  }>(),
  { disabled: false },
)

const emit = defineEmits<{
  (e: 'update:modelValue', value: number): void
}>()

/** 与 tokens.scss `$accent` / `$fill-muted` 对齐（slider 仅接受字面色值）。 */
const activeColor = '#a66b3a'
const trackColor = '#d7d0c4'

function onChange(e: { detail?: { value?: number } }): void {
  if (props.disabled) {
    return
  }
  const raw = e.detail?.value
  const n = typeof raw === 'number' ? raw : Number(raw)
  emit(
    'update:modelValue',
    Number.isFinite(n) ? n : props.modelValue,
  )
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.volume {
  display: flex;
  flex-direction: row;
  align-items: center;
  min-height: max($touch-min, 72px);
}

.volume__head {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 16px;
  flex-shrink: 0;
}

.volume__icon {
  width: 20px;
  height: 20px;
  background-repeat: no-repeat;
  background-size: 100% 100%;
  flex-shrink: 0;
}

.volume__label {
  color: $ink-2;
  font-family: $font-sans;
  font-size: 15px;
  font-weight: 600;
  line-height: 18px;
}

.volume__slider {
  flex: 1;
  margin: 0 12px;
}

.volume__value {
  min-width: 36px;
  color: $ink;
  font-family: $font-sans;
  font-size: 15px;
  font-weight: 700;
  line-height: 18px;
  text-align: right;
}
</style>
