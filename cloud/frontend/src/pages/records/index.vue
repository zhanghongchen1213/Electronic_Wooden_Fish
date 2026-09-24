<template>
  <page-shell :title="RECORDS_COPY.pageTitle">
    <view class="records">
      <!-- 页眉状态胶囊（ready / fail 保留数字时） -->
      <view v-if="statusLabel" class="records__status">
        <text class="records__status-text">{{ statusLabel }}</text>
      </view>

      <view class="records__rule" />

      <!-- loading -->
      <view v-if="phase === 'loading'" class="records__state">
        <text class="records__state-hint">{{ RECORDS_COPY.loadingHint }}</text>
      </view>

      <!-- empty：信任响应 empty，不用全 0 伪装 -->
      <view v-else-if="phase === 'empty'" class="records__state">
        <text class="records__state-title">{{ RECORDS_COPY.emptyTitle }}</text>
        <text class="records__state-hint">{{ RECORDS_COPY.emptyHint }}</text>
      </view>

      <!-- fail：可重试；若有上次数字则叠加 banner -->
      <view v-else-if="phase === 'fail' && !stats" class="records__state">
        <text class="records__state-title">{{ RECORDS_COPY.failTitle }}</text>
        <view class="records__retry" @click="onRetry">
          <text class="records__retry-text">{{ RECORDS_COPY.failAction }}</text>
        </view>
      </view>

      <!-- ready 或 fail+保留数字 -->
      <view v-else-if="stats" class="records__body">
        <view v-if="phase === 'fail'" class="records__fail-banner">
          <text class="records__fail-banner-title">{{ RECORDS_COPY.failTitle }}</text>
          <view class="records__retry records__retry--inline" @click="onRetry">
            <text class="records__retry-text">{{ RECORDS_COPY.failAction }}</text>
          </view>
        </view>

        <stat-card
          :label="RECORDS_COPY.todayLabel"
          :value="todayDisplay"
          :unit="RECORDS_COPY.todayUnit"
        />

        <view class="records__rule records__rule--gap" />

        <stat-ledger-row
          variant="pair"
          left-icon="周"
          :left-label="RECORDS_COPY.weekLabel"
          :left-value="weekDisplay"
          right-icon="月"
          :right-label="RECORDS_COPY.monthLabel"
          :right-value="monthDisplay"
        />

        <view class="records__rule" />

        <stat-ledger-row
          icon="累"
          :label="RECORDS_COPY.totalLabel"
          :value="totalDisplay"
          :unit="RECORDS_COPY.totalUnit"
        />

        <view class="records__rule" />

        <stat-ledger-row
          icon="连"
          :label="RECORDS_COPY.streakLabel"
          :value="streakDisplay"
          :unit="RECORDS_COPY.streakUnit"
        />

        <style-signature variant="records" />
      </view>
    </view>
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.1：记录一级页壳。
 * Story 6.2：一级页门闸 ensureAuthenticated。
 * Story 6.6：五指标统计呈现。
 * Story 6.9：印谱 StyleSignature（装饰非数据）。
 *
 * 裁决 A–G 见 stores/recordsStats.ts。
 * 正式数字只绑 HistoryStats 响应字段；禁止 readingStream / 本地差量加算。
 */
import { computed } from 'vue'
import { onPullDownRefresh, onShow } from '@dcloudio/uni-app'
import { storeToRefs } from 'pinia'
import PageShell from '../../components/page-shell/PageShell.vue'
import StatCard from '../../components/records/StatCard.vue'
import StatLedgerRow from '../../components/records/StatLedgerRow.vue'
import StyleSignature from '../../components/shared/StyleSignature.vue'
import { useAppShellStore } from '../../stores/appShell'
import { useRecordsStatsStore } from '../../stores/recordsStats'
import { ensureAuthenticated } from '../../utils/authGate'
import { RECORDS_COPY } from '../../utils/constants'

const shell = useAppShellStore()
const records = useRecordsStatsStore()
const {
  uiPhase: phase,
  stats,
  todayDisplay,
  weekDisplay,
  monthDisplay,
  totalDisplay,
  streakDisplay,
  statusCapsule,
} = storeToRefs(records)

const statusLabel = computed(() => {
  if (statusCapsule.value === 'pending') {
    return RECORDS_COPY.statusPending
  }
  if (statusCapsule.value === 'confirmed') {
    return RECORDS_COPY.statusConfirmed
  }
  return ''
})

async function loadStats(reason: 'show' | 'pull' | 'retry'): Promise<void> {
  await records.fetchStats({ reason })
  if (reason === 'pull') {
    try {
      uni.stopPullDownRefresh()
    } catch {
      // 非微信宿主忽略
    }
  }
  // ready 与 empty 均属下拉成功；短促「已刷新」对齐 AC #2
  if (
    reason === 'pull' &&
    (records.uiPhase === 'ready' || records.uiPhase === 'empty')
  ) {
    try {
      uni.showToast({ title: RECORDS_COPY.refreshedToast, icon: 'none', duration: 800 })
    } catch {
      // ignore
    }
  }
}

function onRetry(): void {
  void loadStats('retry')
}

onShow(() => {
  if (!ensureAuthenticated()) {
    return
  }
  shell.setCurrentTab('records')
  void loadStats('show')
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
  void loadStats('pull')
})
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.records {
  position: relative;
  padding-bottom: 24px;
}

.records__status {
  position: absolute;
  right: 0;
  top: -42px;
  min-width: 100px;
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

.records__status-text {
  color: $ink-2;
  font-size: 12px;
  font-weight: 600;
  line-height: 1.2;
  text-align: center;
}

.records__rule {
  height: 1px;
  background-color: #9e7b5755;
  width: 100%;
}

.records__rule--gap {
  margin: 16px 0 0;
}

.records__state {
  padding-top: 48px;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8px;
}

.records__state-title {
  color: $ink;
  font-size: 18px;
  font-weight: 600;
  line-height: 1.4;
}

.records__state-hint {
  color: $ink-3;
  font-size: 14px;
  line-height: 1.6;
}

.records__body {
  display: flex;
  flex-direction: column;
  gap: 0;
  padding-top: 16px;
}

.records__fail-banner {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 12px;
}

.records__fail-banner-title {
  color: $ink;
  font-size: 14px;
  font-weight: 600;
}

.records__retry {
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

.records__retry--inline {
  flex-shrink: 0;
}

.records__retry-text {
  color: $accent;
  font-size: 14px;
  font-weight: 600;
  line-height: 1.2;
}
</style>
