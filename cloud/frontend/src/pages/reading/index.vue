<template>
  <page-shell title="阅读">
    <scroll-view
      class="reading-scroll"
      scroll-y
      :scroll-into-view="tailAnchor"
      :scroll-with-animation="true"
      @scroll="onScroll"
    >
      <view
        v-if="showEmpty"
        class="empty"
      >
        <text class="empty__title">{{ copy.emptyTitle }}</text>
        <text class="empty__hint">{{ copy.emptyHint }}</text>
      </view>

      <view
        v-else
        class="live"
      >
        <!-- UX-DR18 / UI_CONTRACT：旧篇在上可折；从头开始后新 round 区块在下 -->
        <reading-archive
          :items="archives"
          @toggle="onToggleArchive"
        />

        <view class="live__meta">
          <text class="live__round">{{ roundLabel }}</text>
          <state-banner
            v-if="bannerTone"
            :tone="bannerTone"
            :title="bannerTitle"
            :copy="bannerCopy"
          />
        </view>

        <view class="live__rule" />

        <reading-line :cursor="displayedCursor" />

        <view
          :id="TAIL_ID"
          class="live__tail"
        />

        <view class="live__rule live__rule--footer" />

        <scripture-progress :cursor="progressCursor" />

        <style-signature variant="reading" />
      </view>
    </scroll-view>

    <confetti-burst
      :pending="celebrationPending"
      @done="onCelebrationDone"
    />
    <modal-done
      :visible="overlayVisible"
      :summary="overlaySummary"
      :busy="actionBusy"
      @restart="onRestart"
      @exit="onExit"
    />
  </page-shell>
</template>

<script setup lang="ts">
/**
 * Story 6.3 + 6.4 + 6.5：在线心经持续阅读流（LIVE / EMPTY / REPLAY / OFFLINE / DONE）。
 *
 * 裁决：
 * - A/E：字形与进度由 round_cursor + STEPS / COUNTS 派生；
 * - C：进页先快照水合，再连 WS；有持久化缺口则 REPLAY；
 * - D：单一 SocketTask；断线 OFFLINE + 契约退避；
 * - F：banner 字面对拍 HTML；
 * - 6.5：DONE banner + OVERLAY + 礼花 + 归档；禁止本地假完成；
 * - 6.9：印谱 StyleSignature（seal/刻线；17 槽基线由 ReadingLine 保留）；
 * - 禁止可点击木鱼、第二进度、本地假敲击。
 *
 * 只消费 4.6 快照 / 5.2 round-action / 5.5 WS 语义 / 5.6 request / 6.2 门闸。
 */
import { computed, ref, watch } from 'vue'
import { onHide, onShow } from '@dcloudio/uni-app'
import PageShell from '../../components/page-shell/PageShell.vue'
import ConfettiBurst from '../../components/reading/ConfettiBurst.vue'
import ModalDone from '../../components/reading/ModalDone.vue'
import ReadingArchive from '../../components/reading/ReadingArchive.vue'
import ReadingLine from '../../components/reading/ReadingLine.vue'
import ScriptureProgress from '../../components/reading/ScriptureProgress.vue'
import StateBanner from '../../components/reading/StateBanner.vue'
import StyleSignature from '../../components/shared/StyleSignature.vue'
import { useAppShellStore } from '../../stores/appShell'
import { useReadingStreamStore } from '../../stores/readingStream'
import { ensureAuthenticated } from '../../utils/authGate'
import { READING_COPY } from '../../utils/constants'
import {
  readingBannerCopy as mapBannerCopy,
  readingBannerTitle as mapBannerTitle,
  readingBannerTone as mapBannerTone,
  readingDoneOverlaySummary,
} from '../../utils/readingBanner'

const TAIL_ID = 'reading-tail'
const copy = READING_COPY
const shell = useAppShellStore()
const stream = useReadingStreamStore()

/** 用户上滑回看时标记；新字到达（displayedCursor 前进）再锚回流尾。 */
const userBrowsing = ref(false)
const tailAnchor = ref('')

const displayedCursor = computed(() => stream.displayedCursor)
/** 弹窗/冻结期进度与摘要绑展示游标，避免权威推进泄漏到可见进度 */
const progressCursor = computed(() => {
  if (stream.overlayVisible || stream.displayFrozen) {
    return stream.displayedCursor
  }
  return stream.roundCursor
})
const showEmpty = computed(() => {
  if (stream.uiPhase === 'done') {
    return false
  }
  return stream.uiPhase === 'empty' || stream.isEmpty
})

const bannerTone = computed(() => mapBannerTone(stream.uiPhase))
const bannerTitle = computed(() => mapBannerTitle(stream.uiPhase))
const bannerCopy = computed(() => mapBannerCopy(stream.uiPhase, progressCursor.value))

const overlayVisible = computed(() => stream.overlayVisible)
const celebrationPending = computed(() => stream.celebrationPending)
const actionBusy = computed(() => stream.actionBusy)
const archives = computed(() => stream.archives)
const overlaySummary = computed(() => readingDoneOverlaySummary(progressCursor.value))

const roundLabel = computed(() => {
  const n = stream.roundId > 0 ? stream.roundId : 1
  return `${copy.roundPrefix} ${n} ${copy.roundSuffix}`
})

function onScroll(e: { detail?: { scrollTop?: number; scrollHeight?: number } }): void {
  const top = e.detail?.scrollTop ?? 0
  const height = e.detail?.scrollHeight ?? 0
  // 粗判：距顶较远视为回看
  if (height > 0 && top + 420 < height - 80) {
    userBrowsing.value = true
  } else {
    userBrowsing.value = false
  }
}

/** EXPERIENCE：默认跟流尾；回看时新字到达应锚回流尾。DONE/弹窗期间可不跟尾新字。 */
function followTailIfNeeded(): void {
  if (stream.overlayVisible || stream.uiPhase === 'done') {
    return
  }
  userBrowsing.value = false
  tailAnchor.value = ''
  setTimeout(() => {
    tailAnchor.value = TAIL_ID
  }, 16)
}

watch(displayedCursor, (next, prev) => {
  if (next > (prev ?? 0)) {
    followTailIfNeeded()
  }
})

function onCelebrationDone(): void {
  stream.clearCelebration()
}

function onToggleArchive(roundId: number): void {
  stream.toggleArchive(roundId)
}

function onRestart(): void {
  void stream.restartRound().then((ok) => {
    if (!ok && stream.lastError) {
      uni.showToast({ title: stream.lastError, icon: 'none' })
    }
  })
}

function onExit(): void {
  void stream.exitRound().then((ok) => {
    if (!ok && stream.lastError) {
      uni.showToast({ title: stream.lastError, icon: 'none' })
    }
  })
}

onShow(() => {
  ensureAuthenticated()
  shell.setCurrentTab('reading')
  void stream.startLiveSession().then(() => {
    followTailIfNeeded()
  })
})

onHide(() => {
  stream.stopLiveSession()
  tailAnchor.value = ''
})
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.reading-scroll {
  height: calc(100vh - #{$safe-top} - 48px);
  box-sizing: border-box;
}

.empty {
  padding-top: 48px;
}

.empty__title {
  display: block;
  color: $ink-3;
  font-size: 14px;
  line-height: 1.6;
}

.empty__hint {
  display: block;
  margin-top: 8px;
  color: $ink-2;
  font-size: 13px;
  line-height: 1.5;
}

.live__meta {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.live__round {
  font-size: 13px;
  line-height: 16px;
  color: $ink-2;
  font-weight: 500;
}

.live__rule {
  height: 1px;
  background-color: #9e7b5755;
  margin-bottom: 16px;
}

.live__rule--footer {
  margin-top: 16px;
  margin-bottom: 16px;
  background-color: $divider;
}

.live__tail {
  height: 1px;
  width: 100%;
}
</style>
