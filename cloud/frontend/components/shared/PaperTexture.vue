<template>
  <view
    class="paper-texture"
    aria-hidden="true"
  />
</template>

<script setup lang="ts">
/**
 * 宣纸纹理层（对拍导出稿每页 [CMP:style-signature][VAR:texture]）。
 * 纹理取自设计稿内联 WebP，并预先与纸面底色 #f4f0e5 合成为不透明 JPEG：
 * 半透明纹理层在部分小程序渲染器/截图像素导出中会单独成层呈现 alpha
 * （棋盘伪影），预合成后任何渲染路径下都与页面底色无缝。
 */
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.paper-texture {
  position: absolute;
  left: $page-pad;
  right: $page-pad;
  top: $safe-top;
  bottom: 0;
  z-index: 0;
  pointer-events: none;
  background-image: url('/static/paper-texture.jpg');
  background-repeat: no-repeat;
  background-size: 100% 100%;
}
</style>
