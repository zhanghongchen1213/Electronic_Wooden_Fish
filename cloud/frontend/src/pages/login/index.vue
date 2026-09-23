<template>
  <view class="login-shell">
    <view class="login-shell__header">
      <text class="login-shell__brand">电子木鱼</text>
    </view>
    <view class="login-shell__body">
      <text class="login-shell__hint">请使用微信授权进入</text>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.1 裁决 F：登录路由壳，路径固定 /pages/login/index。
 * 微信 wx.login → POST /auth/login/wechat-mini → saveTokens → switchTab 阅读属 6.2。
 * 本页不假装「已登录直达」完成。
 */
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.login-shell {
  min-height: 100vh;
  background-color: $bg;
  box-sizing: border-box;
  padding-top: $safe-top;
  padding-left: $page-pad;
  padding-right: $page-pad;
}

.login-shell__brand {
  font-size: 28px;
  font-weight: 600;
  color: $ink;
}

.login-shell__body {
  margin-top: 48px;
}

.login-shell__hint {
  font-size: 14px;
  color: $ink-3;
  line-height: 1.6;
}
</style>
