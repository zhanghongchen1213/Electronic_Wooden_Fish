<template>
  <view class="volume">
    <view class="volume__label-row">
      <view
        v-if="icon"
        class="volume__icon"
        aria-hidden="true"
      >
        <text class="volume__icon-text">{{ icon }}</text>
      </view>
      <text class="volume__label">{{ label }}</text>
    </view>
    <view class="volume__row">
      <slider
        class="volume__slider"
        :value="modelValue"
        :min="0"
        :max="100"
        :step="1"
        :disabled="disabled"
        :activeColor="activeColor"
        :backgroundColor="trackColor"
        :block-size="20"
        @change="onChange"
      />
      <text class="volume__value">{{ modelValue }}</text>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.8：音量滑杆。仅改本地编辑值；不在 changing 过程中打 API（裁决 B）。
 * Story 6.9：可选行首图标（对拍 HTML volume-icon）；触区由 slider + 行高保证 ≥44。
 */
const props = withDefaults(
  defineProps<{
    label: string
    modelValue: number
    disabled?: boolean
    /** 行首字符图标（双通道）。 */
    icon?: string
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
  flex-direction: column;
  gap: 8px;
  min-height: $touch-min;
}

.volume__label-row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
}

.volume__icon {
  width: 20px;
  height: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.volume__icon-text {
  color: $ink-2;
  font-size: 13px;
  font-weight: 700;
  line-height: 1;
}

.volume__label {
  color: $ink;
  font-size: 15px;
  font-weight: 600;
  line-height: 1.2;
}

.volume__row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 12px;
  min-height: $touch-min;
}

.volume__slider {
  flex: 1;
  margin: 0;
}

.volume__value {
  min-width: 36px;
  color: $accent;
  font-size: 15px;
  font-weight: 600;
  text-align: right;
  line-height: 1.2;
}
</style>
