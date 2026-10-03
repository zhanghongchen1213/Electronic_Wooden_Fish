<template>
  <view
    v-if="visible"
    class="modal-done"
    @touchmove.stop.prevent
  >
    <view class="modal-done__mask" />
    <view class="modal-done__card">
      <style-signature variant="overlay" />
      <text class="modal-done__title">{{ title }}</text>
      <text class="modal-done__summary">{{ summary }}</text>
      <view
        class="modal-done__btn modal-done__btn--primary"
        :class="{ 'modal-done__btn--disabled': busy }"
        @click="onRestart"
      >
        <text class="modal-done__btn-label modal-done__btn-label--primary">{{ restartLabel }}</text>
      </view>
      <view
        class="modal-done__btn modal-done__btn--secondary"
        :class="{ 'modal-done__btn--disabled': busy }"
        @click="onExit"
      >
        <text class="modal-done__btn-label modal-done__btn-label--secondary">{{ exitLabel }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.5：OVERLAY.DONE（modal-done）。
 * Story 6.9：复用 StyleSignature overlay（裁决 F）；触区 ≥44×44；busy 时防双发。
 */
import StyleSignature from '../shared/StyleSignature.vue'
import { READING_COPY } from '../../utils/constants'

const props = defineProps<{
  visible: boolean
  summary: string
  busy?: boolean
}>()

const emit = defineEmits<{
  restart: []
  exit: []
}>()

const title = READING_COPY.doneOverlayTitle
const restartLabel = READING_COPY.doneRestart
const exitLabel = READING_COPY.doneExit

function onRestart(): void {
  if (props.busy) {
    return
  }
  emit('restart')
}

function onExit(): void {
  if (props.busy) {
    return
  }
  emit('exit')
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.modal-done {
  position: fixed;
  left: 0;
  top: 0;
  right: 0;
  bottom: 0;
  z-index: 100;
  display: flex;
  align-items: center;
  justify-content: center;
}

.modal-done__mask {
  position: absolute;
  left: 0;
  top: 0;
  right: 0;
  bottom: 0;
  background-color: #2c282399;
}

.modal-done__card {
  position: relative;
  z-index: 1;
  width: 326px;
  padding: 34px 24px 24px;
  box-sizing: border-box;
  background-color: $card;
  border: 1px solid $divider;
  border-radius: 28px;
  display: flex;
  flex-direction: column;
  align-items: center;
}

.modal-done__title {
  font-size: 24px;
  line-height: 29px;
  color: $ink;
  font-weight: 600;
  text-align: center;
}

.modal-done__summary {
  margin-top: 10px;
  margin-bottom: 20px;
  font-size: 14px;
  line-height: 17px;
  color: $ink-2;
  font-weight: 600;
  text-align: center;
}

.modal-done__btn {
  width: 100%;
  min-height: $touch-min;
  border-radius: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.modal-done__btn--primary {
  height: 52px;
  background-color: $ink;
  margin-bottom: 12px;
}

.modal-done__btn--secondary {
  height: 48px;
  background-color: $card;
  border: 1px solid $accent;
}

.modal-done__btn--disabled {
  opacity: 0.55;
  pointer-events: none;
}

.modal-done__btn-label {
  font-weight: 600;
  text-align: center;
}

.modal-done__btn-label--primary {
  font-size: 15px;
  line-height: 18px;
  color: $card;
}

.modal-done__btn-label--secondary {
  font-size: 12px;
  line-height: 14px;
  color: $accent;
}
</style>
