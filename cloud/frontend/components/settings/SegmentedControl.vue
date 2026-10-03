<template>
  <view class="seg" :class="{ 'seg--disabled': disabled }">
    <view class="seg__label-row">
      <view
        v-if="icon"
        class="seg__icon"
        aria-hidden="true"
      >
        <text class="seg__icon-text">{{ icon }}</text>
      </view>
      <text class="seg__label">{{ label }}</text>
    </view>
    <view class="seg__row">
      <view
        v-for="opt in options"
        :key="String(opt.value)"
        class="seg__item"
        :class="{ 'seg__item--active': opt.value === modelValue }"
        @click="onPick(opt.value)"
      >
        <text
          class="seg__item-text"
          :class="{ 'seg__item-text--active': opt.value === modelValue }"
        >{{ opt.label }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.8：亮度 / 熄屏分段控件。选中态可见；触区 ≥44×44。
 * Story 6.9：可选行首图标（对拍 HTML brightness-icon / timeout-icon）。
 */
const props = withDefaults(
  defineProps<{
    label: string
    modelValue: string | number
    options: Array<{ value: string | number; label: string }>
    disabled?: boolean
    /** 行首字符图标（双通道）。 */
    icon?: string
  }>(),
  { disabled: false },
)

const emit = defineEmits<{
  (e: 'update:modelValue', value: string | number): void
}>()

function onPick(value: string | number): void {
  if (props.disabled || value === props.modelValue) {
    return
  }
  emit('update:modelValue', value)
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.seg {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.seg--disabled {
  opacity: 0.72;
  pointer-events: none;
}

.seg__label-row {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
}

.seg__icon {
  width: 20px;
  height: 20px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.seg__icon-text {
  color: $ink-2;
  font-size: 13px;
  font-weight: 700;
  line-height: 1;
}

.seg__label {
  color: $ink;
  font-size: 15px;
  font-weight: 600;
  line-height: 1.2;
}

.seg__row {
  display: flex;
  flex-direction: row;
  gap: 8px;
}

.seg__item {
  flex: 1;
  box-sizing: border-box;
  min-height: $touch-min;
  min-width: $touch-min;
  padding: 10px 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: $fill-muted;
  border-radius: 12px;
}

.seg__item--active {
  background-color: $card;
  outline: 1px solid $accent;
  outline-offset: -0.5px;
}

.seg__item-text {
  color: $ink-2;
  font-size: 14px;
  font-weight: 500;
  line-height: 1.2;
  text-align: center;
}

.seg__item-text--active {
  color: $accent;
  font-weight: 600;
}
</style>
