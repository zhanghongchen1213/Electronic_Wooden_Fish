<template>
  <view class="legal-page">
    <text class="legal-page__title">用户协议</text>
    <text class="legal-page__body">
      欢迎使用电子木鱼。本服务用于查看云端已确认的心经诵读进度。你通过微信授权登录后，将直达唯一绑定设备对应的阅读页。请妥善保管账号，勿将设备交由他人滥用。若不同意本协议，请停止使用本小程序。
    </text>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.2 裁决 D：极简本地协议页；正式法务正文上架前由作者替换。
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
