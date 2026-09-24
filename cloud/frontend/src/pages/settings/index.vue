<template>
  <page-shell :title="SETTINGS_COPY.pageTitle">
    <view class="settings">
      <view class="settings__status">
        <text class="settings__status-text">{{ headerCapsule }}</text>
      </view>

      <view class="settings__rule" />

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
          icon="音"
          :label="SETTINGS_COPY.volumeLabel"
          :model-value="edit.volume"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onVolume"
        />

        <view class="settings__rule settings__rule--soft" />

        <segmented-control
          icon="亮"
          :label="SETTINGS_COPY.brightnessLabel"
          :model-value="edit.brightness"
          :options="brightnessOptions"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onBrightness"
        />

        <view class="settings__rule settings__rule--soft" />

        <segmented-control
          icon="熄"
          :label="SETTINGS_COPY.timeoutLabel"
          :model-value="edit.timeout"
          :options="timeoutOptions"
          :disabled="submitPhase === 'busy'"
          @update:model-value="onTimeout"
        />

        <view class="settings__rule" />

        <apply-status-bar
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
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：设置一级页壳。
 * Story 6.2：一级页门闸 ensureAuthenticated。
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
import { useAppShellStore } from '../../stores/appShell'
import { useSettingsMirrorStore } from '../../stores/settingsMirror'
import { ensureAuthenticated } from '../../utils/authGate'
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
  if (!ensureAuthenticated()) {
    return
  }
  shell.setCurrentTab('settings')
  await settings.fetchSnapshot({ reason: 'show' })
}

onShow(() => {
  void loadShow()
})

onPullDownRefresh(async () => {
  if (!ensureAuthenticated()) {
    uni.stopPullDownRefresh()
    return
  }
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
}

.settings__status {
  align-self: flex-start;
  padding: 6px 12px;
  background-color: $fill-muted;
  border-radius: 999px;
}

.settings__status-text {
  color: $accent;
  font-size: 12px;
  font-weight: 600;
  line-height: 1.2;
}

.settings__rule {
  height: 1px;
  margin: 16px 0;
  background-color: $divider;
}

.settings__rule--soft {
  margin: 14px 0;
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
  color: $accent;
}

.settings__submit-hint {
  margin-top: 12px;
}

.settings__submit-hint-text {
  color: $accent;
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
  color: $accent;
}
</style>
