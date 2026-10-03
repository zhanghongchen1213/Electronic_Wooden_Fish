<template>
  <view class="seg" :class="{ 'seg--disabled': disabled }">
    <view class="seg__head">
      <view
        v-if="icon"
        class="seg__icon"
        aria-hidden="true"
        :style="{ backgroundImage: `url(${iconDataUri(icon, '#8b8177')})` }"
      />
      <text class="seg__label">{{ label }}</text>
    </view>
    <view class="seg__control">
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
 * 2026-10-03 对拍：单行布局（行首 lucide 图标+label 左、分段靠右）；
 * 选中=实心 $accent 白字，未选中=$card 底 + 弱分隔线描边；段高 36 控件高 48 圆角 10 间距 6。
 */
import { iconDataUri, type IconKey } from '../../utils/uiIcons'

const props = withDefaults(
  defineProps<{
    label: string
    modelValue: string | number
    options: Array<{ value: string | number; label: string }>
    disabled?: boolean
    /** 行首图标（设计稿 lucide 键名）。 */
    icon?: IconKey
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
  flex-direction: row;
  align-items: center;
  min-height: 72px;
}

.seg--disabled {
  opacity: 0.72;
  pointer-events: none;
}

.seg__head {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 16px;
  flex-shrink: 0;
}

.seg__icon {
  width: 20px;
  height: 20px;
  background-repeat: no-repeat;
  background-size: 100% 100%;
  flex-shrink: 0;
}

.seg__label {
  color: $ink-2;
  font-family: $font-sans;
  font-size: 15px;
  font-weight: 600;
  line-height: 18px;
}

.seg__control {
  margin-left: auto;
  flex: 0 1 200px;
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 6px;
  padding: 6px 0;
}

.seg__item {
  flex: 1;
  min-width: 50px;
  box-sizing: border-box;
  height: 36px;
  padding: 0 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: $card;
  outline: 1px solid $rule-weak;
  outline-offset: -0.5px;
  border-radius: 10px;
}

.seg__item--active {
  background-color: $accent;
  outline-color: $accent;
}

.seg__item-text {
  color: $ink-2;
  font-family: $font-sans;
  font-size: 12px;
  font-weight: 500;
  line-height: 14px;
  text-align: center;
}

.seg__item-text--active {
  color: #fffdf6;
  font-weight: 600;
}
</style>
