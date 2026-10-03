<template>
  <view
    v-if="items.length > 0"
    class="archive"
  >
    <view
      v-for="item in items"
      :key="item.roundId"
      class="archive__item"
    >
      <view
        class="archive__header"
        @click="emit('toggle', item.roundId)"
      >
        <text class="archive__title">{{ roundLabel(item.roundId) }}</text>
        <text class="archive__action">{{ item.collapsed ? expandLabel : collapseLabel }}</text>
      </view>
      <view
        v-if="!item.collapsed"
        class="archive__body"
      >
        <text class="archive__text">{{ item.chars.join('') }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.5 裁决 D：篇章归档折叠（展示态，非 AD-2 权威）。
 */
import type { ArchiveChapter } from '../../stores/readingStream'
import { READING_COPY } from '../../utils/constants'

defineProps<{
  items: ArchiveChapter[]
}>()

const emit = defineEmits<{
  toggle: [roundId: number]
}>()

const expandLabel = READING_COPY.archiveExpand
const collapseLabel = READING_COPY.archiveCollapse

function roundLabel(roundId: number): string {
  return `${READING_COPY.roundPrefix} ${roundId} ${READING_COPY.roundSuffix}`
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.archive {
  margin-bottom: 20px;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.archive__item {
  background-color: $card;
  border: 1px solid $divider;
  border-radius: 14px;
  overflow: hidden;
}

.archive__header {
  min-height: $touch-min;
  padding: 10px 14px;
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  box-sizing: border-box;
}

.archive__title {
  font-size: 13px;
  line-height: 16px;
  color: $ink-2;
  font-weight: 500;
}

.archive__action {
  font-size: 12px;
  line-height: 14px;
  color: $accent;
  font-weight: 600;
}

.archive__body {
  padding: 0 14px 14px;
}

.archive__text {
  font-size: 14px;
  line-height: 1.7;
  color: $ink;
  word-break: break-all;
}
</style>
