<template>
  <page-shell :title="DEVICE_COPY.pageTitle">
    <view class="device">
      <!-- 页眉状态胶囊 -->
      <view v-if="statusLabel" class="device__status">
        <text class="device__status-text">{{ statusLabel }}</text>
      </view>

      <view class="device__rule" />

      <!-- loading -->
      <view v-if="phase === 'loading'" class="device__state">
        <text class="device__state-hint">{{ DEVICE_COPY.loadingHint }}</text>
      </view>

      <!-- fail 且无缓存：说明是什么+能做什么 -->
      <view v-else-if="phase === 'fail' && !snapshot" class="device__state">
        <text class="device__state-title">{{ DEVICE_COPY.failTitle }}</text>
        <text class="device__state-hint">{{ DEVICE_COPY.failHint }}</text>
        <view class="device__retry" @click="onRetry">
          <text class="device__retry-text">{{ DEVICE_COPY.failAction }}</text>
        </view>
      </view>

      <!-- ready 或 fail+保留快照 -->
      <view v-else-if="snapshot" class="device__body">
        <view v-if="phase === 'fail'" class="device__fail-banner">
          <text class="device__fail-banner-title">{{ DEVICE_COPY.failTitle }}</text>
          <view class="device__retry device__retry--inline" @click="onRetry">
            <text class="device__retry-text">{{ DEVICE_COPY.failAction }}</text>
          </view>
        </view>

        <!-- Story 6.9 裁决 C：队列已满（派生）；可操作=刷新设备状态 -->
        <view v-if="queueFull" class="device__alert" role="status">
          <view class="device__alert-mark" aria-hidden="true">
            <text class="device__alert-mark-text">满</text>
          </view>
          <view class="device__alert-body">
            <text class="device__alert-title">{{ STATE_COPY.queueFullTitle }}</text>
            <text class="device__alert-hint">{{ STATE_COPY.queueFullHint }}</text>
          </view>
        </view>

        <!-- Story 6.9 裁决 D：低电（派生）；icon+文字；禁止充电产品态 -->
        <view v-if="lowBattery" class="device__alert" role="status">
          <view class="device__alert-mark device__alert-mark--warn" aria-hidden="true">
            <text class="device__alert-mark-text">电</text>
          </view>
          <view class="device__alert-body">
            <text class="device__alert-title">{{ STATE_COPY.lowBatteryTitle }}</text>
            <text class="device__alert-hint">{{ STATE_COPY.lowBatteryHint }}</text>
          </view>
        </view>

        <view v-if="weakHint" class="device__weak">
          <text class="device__weak-text">{{ weakHint }}</text>
        </view>

        <!-- 摘要卡：电量 + 4G（图标+文字双通道） -->
        <dev-status-row
          variant="summary"
          :left-label="DEVICE_COPY.batteryLabel"
          :left-value="batteryDisplay"
          :right-label="DEVICE_COPY.networkLabel"
          :right-value="networkDisplay"
          :network-mode="networkModeKey"
        />

        <view class="device__rule device__rule--gap" />

        <dev-status-row
          icon="同"
          :label="DEVICE_COPY.lastSyncLabel"
          :value="lastSyncDisplay"
        />

        <view class="device__rule device__rule--soft" />

        <dev-status-row
          icon="待"
          :label="DEVICE_COPY.pendingLabel"
          :value="pendingDisplay"
        />

        <view class="device__rule device__rule--soft" />

        <dev-status-row
          icon="设"
          :label="DEVICE_COPY.commandLabel"
          :value="commandStatusDisplay"
          :accent="true"
        />

        <view class="device__rule" />

        <!-- state-banner：同步短句 -->
        <view
          class="device__banner"
          :class="{ 'device__banner--fail': phase === 'fail' }"
        >
          <text class="device__banner-text">{{ syncBannerCopy }}</text>
        </view>

        <!-- sync-action 主按钮 -->
        <sync-action-button
          class="device__cta"
          :label="mainCtaLabel"
          :phase="syncAction"
          @click="onMainAction"
        />

        <style-signature variant="device" />
      </view>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：设备一级页壳。
 * Story 6.2：一级页门闸 ensureAuthenticated。
 * Story 6.7：设备快照呈现 + sync-action。
 * Story 6.9：印谱 + 队列已满/低电派生条 + 图标双编码（裁决 A–H / C–D）。
 *
 * 正式状态只来自 GET /sync/snapshot；禁止充电文案、禁止 sync_now、禁止第二 HTTP 栈。
 * 待校时无 wire：不渲染（裁决 E）。
 */
import { computed, onUnmounted, ref } from 'vue'
import { onHide, onPullDownRefresh, onShow } from '@dcloudio/uni-app'
import { storeToRefs } from 'pinia'
import PageShell from '../../components/page-shell/PageShell.vue'
import DevStatusRow from '../../components/device/DevStatusRow.vue'
import SyncActionButton from '../../components/device/SyncActionButton.vue'
import StyleSignature from '../../components/shared/StyleSignature.vue'
import { useAppShellStore } from '../../stores/appShell'
import {
  formatRelativeSyncAt,
  useDeviceStatusStore,
} from '../../stores/deviceStatus'
import { ensureAuthenticated } from '../../utils/authGate'
import { DEVICE_COPY, STATE_COPY } from '../../utils/constants'

const shell = useAppShellStore()
const device = useDeviceStatusStore()
const {
  uiPhase: phase,
  snapshot,
  syncAction,
  lastSyncedAt,
  batteryDisplay,
  networkDisplay,
  networkModeKey,
  pendingDisplay,
  commandStatusDisplay,
  statusCapsule,
  syncBannerCopy,
  mainCtaLabel,
  weakDeviceHint: weakHint,
  queueFull,
  lowBattery,
} = storeToRefs(device)

/** 驱动「N 分钟前」推进；非 wire 时钟。 */
const clockMs = ref(Date.now())
let clockTimer: ReturnType<typeof setInterval> | null = null

function startClock(): void {
  clockMs.value = Date.now()
  if (clockTimer != null) {
    return
  }
  clockTimer = setInterval(() => {
    clockMs.value = Date.now()
  }, 30_000)
}

function stopClock(): void {
  if (clockTimer != null) {
    clearInterval(clockTimer)
    clockTimer = null
  }
}

const lastSyncDisplay = computed(() =>
  formatRelativeSyncAt(lastSyncedAt.value, clockMs.value),
)

const statusLabel = computed(() => {
  if (statusCapsule.value === 'pending') {
    return DEVICE_COPY.statusPending
  }
  if (statusCapsule.value === 'connected') {
    return DEVICE_COPY.statusConnected
  }
  return ''
})

async function loadSnapshot(
  reason: 'show' | 'pull' | 'refresh' | 'retry' | 'sync_intent',
): Promise<void> {
  const { joined } = await device.fetchSnapshot({ reason })
  if (reason === 'pull') {
    try {
      uni.stopPullDownRefresh()
    } catch {
      // 非微信宿主忽略
    }
  }
  // join 既有单飞时不再弹 toast，避免连点/下拉重叠多次「已刷新」
  if (
    !joined
    && (reason === 'pull' || reason === 'refresh' || reason === 'sync_intent')
    && device.uiPhase === 'ready'
  ) {
    try {
      uni.showToast({
        title: DEVICE_COPY.refreshedToast,
        icon: 'none',
        duration: 800,
      })
    } catch {
      // ignore
    }
  }
}

function onRetry(): void {
  void loadSnapshot('retry')
}

function onMainAction(): void {
  if (device.uiPhase === 'fail' || device.syncAction === 'fail') {
    void loadSnapshot('retry')
    return
  }
  // 刷新与「立即同步」意图共用同一单飞（裁决 G）
  void loadSnapshot('refresh')
}

onShow(() => {
  if (!ensureAuthenticated()) {
    return
  }
  shell.setCurrentTab('device')
  startClock()
  void loadSnapshot('show')
})

onHide(() => {
  stopClock()
})

onUnmounted(() => {
  stopClock()
})

onPullDownRefresh(() => {
  if (!ensureAuthenticated()) {
    try {
      uni.stopPullDownRefresh()
    } catch {
      // ignore
    }
    return
  }
  void loadSnapshot('pull')
})
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.device {
  position: relative;
  padding-bottom: 24px;
}

.device__status {
  position: absolute;
  right: 0;
  top: -42px;
  min-width: 84px;
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

.device__status-text {
  color: $ink-2;
  font-size: 12px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.device__rule {
  height: 1px;
  background-color: $divider;
  width: 100%;
}

.device__rule--soft {
  background-color: $divider;
}

.device__rule--gap {
  margin: 16px 0 0;
}

.device__state {
  padding-top: 48px;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8px;
}

.device__state-title {
  color: $ink;
  font-size: 18px;
  font-weight: 600;
  line-height: 1.4;
}

.device__state-hint {
  color: $ink-3;
  font-size: 14px;
  line-height: 1.6;
}

.device__body {
  display: flex;
  flex-direction: column;
  gap: 0;
  padding-top: 16px;
}

.device__alert {
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  gap: 10px;
  margin-bottom: 12px;
  padding: 12px 14px;
  box-sizing: border-box;
  background-color: $card;
  outline: 1px solid $divider;
  outline-offset: -0.5px;
  border-radius: 12px;
}

.device__alert-mark {
  width: 28px;
  height: 28px;
  border-radius: 6px;
  background-color: $fill-muted;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.device__alert-mark--warn {
  background-color: rgba(166, 107, 58, 0.18);
}

.device__alert-mark-text {
  color: $accent;
  font-size: 12px;
  font-weight: 700;
  line-height: 1;
}

.device__alert-body {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.device__alert-title {
  color: $ink;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.3;
}

.device__alert-hint {
  color: $ink-2;
  font-size: 13px;
  line-height: 1.4;
}

.device__weak {
  margin-bottom: 12px;
}

.device__weak-text {
  color: $ink-3;
  font-size: 13px;
  line-height: 1.4;
}

.device__fail-banner {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 12px;
}

.device__fail-banner-title {
  color: $ink;
  font-size: 14px;
  font-weight: 600;
}

.device__retry {
  min-width: $touch-min;
  min-height: $touch-min;
  padding: 10px 16px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: $card;
  outline: 1px solid $divider;
  outline-offset: -0.5px;
  border-radius: 12px;
}

.device__retry--inline {
  flex-shrink: 0;
}

.device__retry-text {
  color: $accent;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.2;
}

.device__banner {
  box-sizing: border-box;
  width: 100%;
  min-height: 46px;
  margin-top: 16px;
  padding: 12px 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: $fill-muted;
  border-radius: 12px;
}

.device__banner-text {
  color: $ink-2;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.device__cta {
  margin-top: 18px;
}
</style>
