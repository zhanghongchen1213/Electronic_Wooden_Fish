<template>
  <view class="char-focus">
    <text class="char-focus__glyph">{{ char }}</text>
    <view class="char-focus__underline" />
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.3 / UX-DR15：最新已确认字。
 * 28px + colors.mini.focus（#a66b3a）+ 持续 2px×20 下划线。
 */
defineProps<{
  char: string
}>()
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.char-focus {
  width: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: flex-end;
}

.char-focus__glyph {
  font-size: 28px;
  line-height: 32px;
  color: $accent;
  font-family: 'Noto Serif SC', 'Songti SC', serif;
  font-weight: 600;
}

.char-focus__underline {
  width: 20px;
  height: 2px;
  background-color: $accent;
  margin-top: 0;
}
</style>
