<template>
  <view
    class="sync-action"
    :class="{
      'sync-action--busy': phase === 'busy',
      'sync-action--pending': phase === 'pending',
      'sync-action--ok': phase === 'ok',
      'sync-action--fail': phase === 'fail',
    }"
    @click="onTap"
  >
    <view
      class="sync-action__icon"
      aria-hidden="true"
      :style="{ backgroundImage: `url(${iconDataUri('refresh-cw', iconColor)})` }"
    />
    <text class="sync-action__label">{{ label }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.7：sync-action 主按钮。
 * 2026-10-03 对拍：图标对齐导出稿 [VAR:icon] lucide refresh-cw 18×18；
 * fail 态文字用 $danger（对齐状态卡「同步失败」#a64c3e），品牌强调态仍 $accent。
 * 触区 ≥44×44；busy 时仍可点但调用方靠 store 单飞吞并发。
 */
import { computed } from 'vue'
import type { SyncActionPhase } from '../../stores/deviceStatus'
import { iconDataUri } from '../../utils/uiIcons'

const props = defineProps<{
  label: string
  phase: SyncActionPhase
}>()

const emit = defineEmits<{
  (e: 'click'): void
}>()

const iconColor = computed(() => {
  if (props.phase === 'pending') return '#a66b3a'
  if (props.phase === 'fail') return '#a64c3e'
  return '#fffdf6'
})

function onTap(): void {
  void props
  emit('click')
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.sync-action {
  box-sizing: border-box;
  width: 100%;
  min-height: 52px;
  min-width: $touch-min;
  padding: 14px 16px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  gap: 8px;
  background-color: $ink;
  border-radius: 16px;
}

.sync-action--busy {
  opacity: 0.72;
}

.sync-action--pending {
  background-color: $card;
  outline: 1px solid $accent;
  outline-offset: -0.5px;
}

.sync-action--ok {
  background-color: $ink;
}

.sync-action--fail {
  background-color: $card;
  outline: 1px solid $rule-strong;
  outline-offset: -0.5px;
}

.sync-action__icon {
  width: 18px;
  height: 18px;
  background-repeat: no-repeat;
  background-size: 100% 100%;
  flex-shrink: 0;
}

.sync-action__label {
  color: $card;
  font-family: $font-sans;
  font-size: 15px;
  font-weight: 600;
  line-height: 18px;
  text-align: center;
}

.sync-action--pending .sync-action__label {
  color: $accent;
}

.sync-action--fail .sync-action__label {
  color: $danger;
}
</style>
