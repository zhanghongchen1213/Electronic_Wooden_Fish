<template>
  <view class="page-shell">
    <paper-texture />
    <view class="page-header">
      <view class="page-header__row">
        <text class="page-header__title">{{ title }}</text>
        <slot name="header-right" />
      </view>
    </view>
    <view class="page-body">
      <slot />
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.1 裁决 C：page-header 自 y=62 起；左右 page_pad=20；无业务假数据。
 * 2026-10-03 对拍裁决：底部改为 uni.hideTabBar + 页面内嵌自绘 BottomNav，
 * pages.json tabBar 仅承载 switchTab 路由，原生栏不再显示；
 * 页头为「标题左 + 状态胶囊右」行布局（对拍导出稿 page-header）。
 * 宣纸纹理层铺内容区，业务内容 z-index 压于其上。
 */
import PaperTexture from '../shared/PaperTexture.vue'

defineProps<{
  title: string
}>()
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.page-shell {
  position: relative;
  min-height: 100vh;
  background-color: $bg;
  box-sizing: border-box;
}

.page-header {
  position: relative;
  z-index: 1;
  padding: $safe-top $page-pad 22px;
}

.page-header__row {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  min-height: 28px;
}

.page-header__title {
  font-family: $font-serif;
  font-size: 22px;
  line-height: 26px;
  font-weight: 600;
  color: $ink;
}

.page-body {
  position: relative;
  z-index: 1;
  padding-left: $page-pad;
  padding-right: $page-pad;
}
</style>
