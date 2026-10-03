<template>
  <page-shell :title="SETTINGS_COPY.pageTitle">
    <template #header-right>
      <!-- 页眉「设备镜像」胶囊，固定于标题行右侧 -->
      <view class="settings__status">
        <text class="settings__status-text">{{ headerCapsule }}</text>
      </view>
    </template>

    <view class="settings">
      <view class="settings__rule settings__rule--head" />

      <view v-if="phase === 'loading'" class="settings__state">
        <text class="settings__state-hint">{{ SETTINGS_COPY.loadingHint }}</text>
      </view>

      <view v-else-if="phase === 'fail' && !snapshot" class="settings__state">
        <text class="settings__state-title">{{ SETTINGS_COPY.failTitle }}</text>
        <text class="settings__state-hint">{{ SETTINGS_COPY.weakEmpty }}</text>
        <view class="settings__retry" @click="onRetry">
          <text class="settings__retry-text">{{ SETTINGS_COPY.failAction }}</text>
        </view>
      </view>

      <view v-else class="settings__body">
        <view v-if="phase === 'fail'" class="settings__fail-banner">
          <text class="settings__fail-banner-text">{{ SETTINGS_COPY.failTitle }}</text>
          <view class="settings__retry settings__retry--inline" @click="onRetry">
            <text class="settings__retry-text">{{ SETTINGS_COPY.failAction }}</text>
          </view>
        </view>

        <volume-slider
          icon="volume-2"
          :label="SETTINGS_COPY.volumeLabel"
          :model-value="edit.volume"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onVolume"
        />

        <view class="settings__rule settings__rule--soft" />

        <segmented-control
          icon="sun"
          :label="SETTINGS_COPY.brightnessLabel"
          :model-value="edit.brightness"
          :options="brightnessOptions"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onBrightness"
        />

        <view class="settings__rule settings__rule--soft" />

        <segmented-control
          icon="moon"
          :label="SETTINGS_COPY.timeoutLabel"
          :model-value="edit.timeout"
          :options="timeoutOptions"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onTimeout"
        />

        <view class="settings__rule settings__rule--soft" />

        <apply-status-bar
          class="settings__apply"
          :status="applyStatus"
          :label="applyStatusDisplay"
        />

        <view
          v-if="submitPhase === 'fail' && lastError"
          class="settings__submit-hint"
        >
          <text class="settings__submit-hint-text">{{ lastError }}</text>
        </view>

        <view
          class="settings__cta"
          :class="{
            'settings__cta--busy': submitPhase === 'busy',
            'settings__cta--fail': submitPhase === 'fail',
          }"
          @click="onSave"
        >
          <text class="settings__cta-label">{{ primaryCtaLabel }}</text>
        </view>

        <style-signature variant="settings" />
      </view>
    </view>

    <bottom-nav />
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：设置一级页壳。
 * Story 6.8：设置镜像三控件 + 待设备应用/已生效 + 保存并下发。
 * Story 6.9：印谱 + 触区终验（滑杆行 $touch-min）。
 *
 * 裁决 A–H 见 stores/settingsMirror.ts。
 * 正式权威只来自 snapshot / command 响应；禁止本地假 ACK、禁止第二 HTTP 栈。
 */
import { computed } from 'vue'
import { onPullDownRefresh, onShow } from '@dcloudio/uni-app'
import { storeToRefs } from 'pinia'
import PageShell from '../../components/page-shell/PageShell.vue'
import ApplyStatusBar from '../../components/settings/ApplyStatusBar.vue'
import SegmentedControl from '../../components/settings/SegmentedControl.vue'
import VolumeSlider from '../../components/settings/VolumeSlider.vue'
import StyleSignature from '../../components/shared/StyleSignature.vue'
import BottomNav from '../../components/bottom-nav/BottomNav.vue'
import { useAppShellStore } from '../../stores/appShell'
import { useSettingsMirrorStore } from '../../stores/settingsMirror'
import { SETTINGS_COPY } from '../../utils/constants'
import type { BrightnessWire, TimeoutWire } from '../../types/sync'

const shell = useAppShellStore()
const settings = useSettingsMirrorStore()
const {
  uiPhase: phase,
  snapshot,
  edit,
  applyStatus,
  submitPhase,
  lastError,
} = storeToRefs(settings)

const headerCapsule = computed(() => settings.headerCapsule)
const applyStatusDisplay = computed(() => settings.applyStatusDisplay)
const primaryCtaLabel = computed(() => settings.primaryCtaLabel)

const brightnessOptions: Array<{ value: BrightnessWire; label: string }> = [
  { value: 'low', label: SETTINGS_COPY.brightnessLow },
  { value: 'mid', label: SETTINGS_COPY.brightnessMid },
  { value: 'high', label: SETTINGS_COPY.brightnessHigh },
]

const timeoutOptions: Array<{ value: TimeoutWire; label: string }> = [
  { value: 5, label: SETTINGS_COPY.timeout5 },
  { value: 15, label: SETTINGS_COPY.timeout15 },
  { value: 30, label: SETTINGS_COPY.timeout30 },
]

async function loadShow(): Promise<void> {
  shell.setCurrentTab('settings')
  await settings.fetchSnapshot({ reason: 'show' })
}

onShow(() => {
  uni.hideTabBar({ fail: () => {} })
  void loadShow()
})

onPullDownRefresh(async () => {
  await settings.fetchSnapshot({ reason: 'pull' })
  uni.stopPullDownRefresh()
})

function onVolume(value: number): void {
  if (settings.submitPhase === 'busy') {
    return
  }
  settings.setVolume(value)
}

function onBrightness(value: string | number): void {
  if (settings.submitPhase === 'busy') {
    return
  }
  settings.setBrightness(value as BrightnessWire)
}

function onTimeout(value: string | number): void {
  if (settings.submitPhase === 'busy') {
    return
  }
  settings.setTimeoutSeconds(value as TimeoutWire)
}

async function onRetry(): Promise<void> {
  await settings.fetchSnapshot({ reason: 'retry' })
}

async function onSave(): Promise<void> {
  if (settings.submitPhase === 'busy') {
    return
  }
  await settings.submitEdit()
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.settings {
  display: flex;
  flex-direction: column;
  gap: 0;
  padding-bottom: calc(#{$nav-h} + 20px + env(safe-area-inset-bottom));
}

.settings__status {
  min-width: 88px;
  height: 28px;
  padding: 0 10px;
  box-sizing: border-box;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: $fill-muted;
  border-radius: 14px;
}

.settings__status-text {
  color: $ink-2;
  font-family: $font-sans;
  font-size: 12px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.settings__rule {
  height: 1px;
  margin: 16px 0;
  background-color: $rule-strong;
}

/* 页头线：设计稿设置区起于更低位置（标题盒底 +46） */
.settings__rule--head {
  margin: 24px 0 0;
}

/* 行后弱线：紧贴 72 行盒（对拍导出稿 rule @217/289/361） */
.settings__rule--soft {
  margin: 0;
  background-color: $rule-weak;
}

.settings__apply {
  margin-top: 24px;
}

.settings__state {
  padding-top: 32px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
}

.settings__state-title {
  color: $ink;
  font-size: 16px;
  font-weight: 600;
}

.settings__state-hint {
  color: $ink-3;
  font-size: 14px;
  line-height: 1.6;
}

.settings__body {
  display: flex;
  flex-direction: column;
}

.settings__fail-banner {
  margin-bottom: 12px;
  padding: 12px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  background-color: $fill-muted;
  border-radius: 12px;
}

.settings__fail-banner-text {
  color: $ink;
  font-size: 14px;
  font-weight: 600;
}

.settings__retry {
  min-height: $touch-min;
  min-width: $touch-min;
  padding: 10px 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: $ink;
  border-radius: 12px;
}

.settings__retry--inline {
  background-color: transparent;
  outline: 1px solid $accent;
  outline-offset: -0.5px;
}

.settings__retry-text {
  color: $card;
  font-size: 14px;
  font-weight: 600;
}

.settings__retry--inline .settings__retry-text {
  color: $danger;
}

.settings__submit-hint {
  margin-top: 12px;
}

.settings__submit-hint-text {
  color: $danger;
  font-family: $font-sans;
  font-size: 13px;
  line-height: 1.4;
}

.settings__cta {
  box-sizing: border-box;
  width: 100%;
  margin-top: 20px;
  min-height: 52px;
  min-width: $touch-min;
  padding: 14px 16px;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  background-color: $ink;
  border-radius: 16px;
}

.settings__cta--busy {
  opacity: 0.72;
}

.settings__cta--fail {
  background-color: $card;
  outline: 1px solid $divider;
  outline-offset: -0.5px;
}

.settings__cta-label {
  color: $card;
  font-size: 15px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.settings__cta--fail .settings__cta-label {
  color: $danger;
}
</style>
