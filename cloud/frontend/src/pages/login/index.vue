<template>
  <view class="login-page">
    <view class="login-page__header">
      <text class="login-title">{{ copy.title }}</text>
      <text class="login-copy">{{ copy.summary }}</text>
    </view>

    <view class="login-page__hero" aria-hidden="true">
      <style-signature variant="login" />
    </view>

    <view v-if="showPermError" class="login-page__perm" role="alert">
      <text class="login-page__perm-title">{{ auth.permErrorMessage || copy.permTitle }}</text>
      <text class="login-page__perm-hint">{{ copy.permAction }}</text>
    </view>
    <view v-else class="login-page__assist">
      <text class="login-page__assist-text">{{ copy.assist }}</text>
    </view>

    <view class="login-page__agree" @click="toggleAgree">
      <view class="login-page__checkbox" :class="{ 'is-checked': agreed }">
        <text v-if="agreed" class="login-page__check-mark">✓</text>
      </view>
      <text class="login-page__agree-text">
        {{ copy.agreementPrefix }}
        <text class="login-page__link" @click.stop="openUserAgreement">{{ copy.userAgreement }}</text>
        与
        <text class="login-page__link" @click.stop="openPrivacy">{{ copy.privacyPolicy }}</text>
      </text>
    </view>

    <button
      class="login-action"
      :class="{ 'is-loading': actionBusy }"
      :disabled="actionBusy"
      :loading="actionBusy"
      hover-class="login-action--hover"
      @click="onLoginTap"
    >
      {{ actionLabel }}
    </button>
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.2：LOGIN BASE + PERM_ERROR。
 * Story 6.9：印谱 StyleSignature（对拍 HTML hero-seal）；权限态非纯色（卡片+标题+动作）。
 * 裁决 A：成功 switchTab 阅读；失败留本页权限态。
 * 裁决 C：权限文案优先信封 message，否则「权限错误」。
 * 裁决 D：协议勾选硬门槛；未勾选不可登录。
 * 裁决 F：成功后阅读页仍可空态，不拉快照。
 * 防连点：busy 覆盖 uni.login 等待窗；auth.loading 覆盖换票请求窗。
 */
import { computed, ref } from 'vue'
import StyleSignature from '../../components/shared/StyleSignature.vue'
import { useUserAuthStore } from '../../stores/userAuth'
import { navigateToReadingAfterLogin } from '../../utils/authGate'
import { LEGAL_PAGE_PATHS, LOGIN_COPY } from '../../utils/constants'

const copy = LOGIN_COPY
const auth = useUserAuthStore()
const agreed = ref(false)
/** uni.login 进行中防连点（早于 store.loading）。 */
const busy = ref(false)

const showPermError = computed(() => auth.loginPhase === 'perm_error')
const actionBusy = computed(() => busy.value || auth.loading)
const actionLabel = computed(() =>
  showPermError.value ? copy.permAction : copy.action,
)

function toggleAgree() {
  agreed.value = !agreed.value
}

function openUserAgreement() {
  uni.navigateTo({ url: LEGAL_PAGE_PATHS.userAgreement })
}

function openPrivacy() {
  uni.navigateTo({ url: LEGAL_PAGE_PATHS.privacyPolicy })
}

function toast(title: string) {
  uni.showToast({ title, icon: 'none' })
}

async function onLoginTap() {
  if (actionBusy.value) {
    return
  }
  if (!agreed.value) {
    toast(copy.agreeHint)
    return
  }

  busy.value = true
  try {
    const loginResult = await new Promise<UniApp.LoginRes>((resolve, reject) => {
      uni.login({
        provider: 'weixin',
        success: resolve,
        fail: reject,
      })
    })
    const code = loginResult?.code
    if (!code) {
      auth.loginPhase = 'perm_error'
      auth.permErrorMessage = copy.permTitle
      return
    }
    const ok = await auth.loginWithWxCode(code)
    if (ok) {
      navigateToReadingAfterLogin()
    }
  } catch {
    auth.loginPhase = 'perm_error'
    auth.permErrorMessage = copy.permTitle
  } finally {
    busy.value = false
  }
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.login-page {
  min-height: 100vh;
  background-color: $bg;
  box-sizing: border-box;
  padding-top: $safe-top;
  padding-left: $page-pad;
  padding-right: $page-pad;
  padding-bottom: 48px;
  display: flex;
  flex-direction: column;
}

.login-page__header {
  margin-top: 8px;
}

.login-title {
  display: block;
  font-size: 28px;
  font-weight: 600;
  color: $ink;
  line-height: 1.3;
}

.login-copy {
  display: block;
  margin-top: 12px;
  font-size: 14px;
  color: $ink-2;
  line-height: 1.6;
}

.login-page__hero {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 180px;
  margin: 24px 0;
}

.login-page__assist {
  margin-bottom: 20px;
}

.login-page__assist-text {
  font-size: 13px;
  color: $ink-3;
  line-height: 1.5;
}

.login-page__perm {
  margin-bottom: 20px;
  padding: 12px 14px;
  background: $card;
  border-radius: 8px;
  border: 1px solid $divider;
}

.login-page__perm-title {
  display: block;
  font-size: 15px;
  font-weight: 600;
  color: $ink;
  line-height: 1.4;
}

.login-page__perm-hint {
  display: block;
  margin-top: 6px;
  font-size: 13px;
  color: $ink-2;
  line-height: 1.5;
}

.login-page__agree {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  margin-bottom: 16px;
  min-height: $touch-min;
}

.login-page__checkbox {
  width: 18px;
  height: 18px;
  margin-top: 2px;
  border: 1px solid $ink-2;
  border-radius: 4px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  background: $card;
}

.login-page__checkbox.is-checked {
  background: $ink;
  border-color: $ink;
}

.login-page__check-mark {
  color: $card;
  font-size: 12px;
  line-height: 1;
}

.login-page__agree-text {
  font-size: 12px;
  color: $ink-2;
  line-height: 1.5;
  flex: 1;
}

.login-page__link {
  color: $accent;
}

.login-action {
  width: 350px;
  max-width: 100%;
  height: 56px;
  line-height: 56px;
  margin: 0 auto;
  padding: 0;
  border: none;
  border-radius: 12px;
  background-color: $ink;
  color: $card;
  font-size: 16px;
  font-weight: 500;
  text-align: center;
}

.login-action::after {
  border: none;
}

.login-action--hover {
  opacity: 0.88;
}

.login-action.is-loading {
  opacity: 0.7;
}
</style>
