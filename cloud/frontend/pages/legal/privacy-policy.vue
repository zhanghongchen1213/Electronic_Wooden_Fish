<template>
  <view class="legal-page">
    <text class="legal-page__title">隐私政策</text>
    <text class="legal-page__body">
      电子木鱼仅通过微信登录凭证向自有服务换取会话，用于识别你的唯一设备并展示已确认进度。我们不会在小程序本地保存微信会话密钥，也不会将身份材料写入页面文案或普通日志。你可以随时停止使用本服务；更多细节以运营方正式发布的隐私说明为准。
    </text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.2 裁决 D：极简本地隐私页；正式法务正文上架前由作者替换。
 */
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.legal-page {
  min-height: 100vh;
  background-color: $bg;
  box-sizing: border-box;
  padding: $safe-top $page-pad 48px;
}

.legal-page__title {
  display: block;
  font-size: 22px;
  font-weight: 600;
  color: $ink;
  margin-bottom: 16px;
}

.legal-page__body {
  display: block;
  font-size: 14px;
  color: $ink-2;
  line-height: 1.7;
}
</style>
