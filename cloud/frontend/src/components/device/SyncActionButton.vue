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
    <view class="sync-action__icon" aria-hidden="true">
      <text class="sync-action__icon-text">↻</text>
    </view>
    <text class="sync-action__label">{{ label }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.7：sync-action 主按钮。
 * Story 6.9：对拍 HTML [VAR:icon]——字符刷新标 + 文案双通道。
 * 触区 ≥44×44；busy 时仍可点但调用方靠 store 单飞吞并发。
 */
import type { SyncActionPhase } from '../../stores/deviceStatus'

const props = defineProps<{
  label: string
  phase: SyncActionPhase
}>()

const emit = defineEmits<{
  (e: 'click'): void
}>()

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
  outline: 1px solid $divider;
  outline-offset: -0.5px;
}

.sync-action__icon {
  width: 18px;
  height: 18px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.sync-action__icon-text {
  color: $card;
  font-size: 16px;
  font-weight: 700;
  line-height: 1;
}

.sync-action__label {
  color: $card;
  font-size: 15px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.sync-action--pending .sync-action__label,
.sync-action--fail .sync-action__label,
.sync-action--pending .sync-action__icon-text,
.sync-action--fail .sync-action__icon-text {
  color: $accent;
}
</style>
