<template>
  <view class="reading-line">
    <view
      v-for="(row, rowIndex) in rows"
      :key="`row-${rowIndex}`"
      class="reading-line__row"
    >
      <view
        v-for="(ch, colIndex) in row"
        :key="`c-${rowIndex}-${colIndex}`"
        class="reading-line__slot"
      >
        <char-focus
          v-if="isFocus(rowIndex, colIndex)"
          :char="ch"
        />
        <text
          v-else
          class="reading-line__char reading-line__char--muted"
        >{{ ch }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.3 / UX-DR15：17 槽行列表。
 * 槽宽 20、行步进 46（cell 42 + 间距 4，对拍导出稿行 1@174/行 2@220）；
 * 正文 18px/32px Serif 500 墨色；最新字由 CharFocus（28 + 下划线）。
 * 禁止预览未来经文：只渲染已确认 display 槽。
 */
import { computed } from 'vue'
import { displayCharsUpTo, layoutRows, SLOTS_PER_LINE } from '../../utils/scriptureProjection'
import CharFocus from './CharFocus.vue'

const props = defineProps<{
  cursor: number
}>()

const chars = computed(() => displayCharsUpTo(props.cursor))
const rows = computed(() => layoutRows(chars.value, SLOTS_PER_LINE))

function flatIndex(rowIndex: number, colIndex: number): number {
  return rowIndex * SLOTS_PER_LINE + colIndex
}

function isFocus(rowIndex: number, colIndex: number): boolean {
  const len = chars.value.length
  if (len === 0) {
    return false
  }
  return flatIndex(rowIndex, colIndex) === len - 1
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.reading-line {
  width: 100%;
}

.reading-line__row {
  display: flex;
  flex-direction: row;
  height: 42px;
  align-items: flex-end;
  margin-bottom: 4px;
}

.reading-line__slot {
  width: 20px;
  height: 42px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-end;
  flex-shrink: 0;
}

.reading-line__char {
  font-family: $font-serif;
  font-size: 18px;
  line-height: 32px;
  font-weight: 500;
  color: $ink;
}

.reading-line__char--muted {
  color: $ink;
}
</style>
