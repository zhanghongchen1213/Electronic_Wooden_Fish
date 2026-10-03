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
  position: relative;
  width: 20px;
  height: 42px;
  display: block;
}

.char-focus__glyph {
  display: block;
  font-family: $font-serif;
  font-size: 28px;
  line-height: 36px;
  color: $accent;
  font-weight: 600;
  text-align: center;
}

/* 下划线钉在 cell 内 top 38（对拍导出稿焦点字下划线 @ (·,38)） */
.char-focus__underline {
  position: absolute;
  left: 0;
  top: 38px;
  width: 20px;
  height: 2px;
  background-color: $accent;
}
</style>
