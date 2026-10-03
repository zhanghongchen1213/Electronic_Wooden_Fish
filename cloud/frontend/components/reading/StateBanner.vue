<template>
  <view
    class="state-banner"
    :class="toneClass"
  >
    <text class="state-banner__title">{{ title }}</text>
    <text
      v-if="copy"
      class="state-banner__copy"
    >{{ copy }}</text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.4 + 6.5：阅读态 banner（LIVE / REPLAY / OFFLINE / DONE）。
 * 颜色不是唯一通道——必须有文字；字面来自 READING_COPY / HTML。
 */
import { computed } from 'vue'

const props = defineProps<{
  tone: 'live' | 'replay' | 'offline' | 'done'
  title: string
  copy?: string
}>()

const toneClass = computed(() => `state-banner--${props.tone}`)
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.state-banner {
  min-width: 76px;
  min-height: 28px;
  padding: 4px 12px;
  border-radius: 14px;
  background-color: $fill-muted;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 2px;
}

.state-banner--live {
  background-color: $fill-muted;
}

.state-banner--replay {
  background-color: #e8dcc8;
}

.state-banner--offline {
  background-color: #d0c8bc;
}

.state-banner--done {
  background-color: #f3e6d0;
  min-width: 120px;
  align-items: flex-start;
  padding: 8px 12px;
}

.state-banner--done .state-banner__title {
  color: $accent;
}

.state-banner__title {
  font-size: 12px;
  line-height: 14px;
  color: $ink-2;
  font-weight: 600;
}

.state-banner__copy {
  font-size: 10px;
  line-height: 12px;
  color: $ink-3;
  font-weight: 500;
}
</style>
