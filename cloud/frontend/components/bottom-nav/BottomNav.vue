<template>
  <!--
    2026-10-03 对拍裁决：uni.hideTabBar + 页面内嵌自绘底栏（对拍导出稿
    cmp_bottom_nav_rich：350×56 圆角 28 卡片，上方 1px 强分隔线）。
    pages.json tabBar 仅承载 switchTab 路由；触区 ≥44×44。
  -->
  <view
    class="bottom-nav"
    role="navigation"
  >
    <view class="bottom-nav__rule" />
    <view class="bottom-nav__card">
      <view
        v-for="item in items"
        :key="item.key"
        class="bottom-nav__item"
        :class="{ 'is-active': item.key === activeTab }"
        @click="onSelect(item.key)"
      >
        <view
          class="bottom-nav__icon"
          :style="{ backgroundImage: `url(${iconDataUri(item.icon, item.key === activeTab ? '#a66b3a' : '#8b8177')})` }"
        />
        <text class="bottom-nav__label">{{ item.label }}</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { NAV_TAB_KEYS, NAV_TAB_LABELS, NAV_TAB_ROUTES, type PrimaryTabKey } from '../../utils/constants'
import { useAppShellStore } from '../../stores/appShell'
import { iconDataUri, type IconKey } from '../../utils/uiIcons'

const TAB_ICONS: Record<PrimaryTabKey, IconKey> = {
  reading: 'book-open',
  records: 'activity',
  device: 'radio',
  settings: 'sliders-horizontal',
}

const props = defineProps<{
  current?: PrimaryTabKey
}>()

const shell = useAppShellStore()

const items = NAV_TAB_KEYS.map((key, index) => ({
  key,
  label: NAV_TAB_LABELS[index],
  icon: TAB_ICONS[key],
}))

const activeTab = computed(() => props.current ?? shell.currentTab)

function onSelect(key: PrimaryTabKey) {
  if (key === activeTab.value) {
    return
  }
  shell.setCurrentTab(key)
  uni.switchTab({ url: NAV_TAB_ROUTES[key] })
}
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.bottom-nav {
  position: fixed;
  left: $page-pad;
  right: $page-pad;
  bottom: calc(20px + env(safe-area-inset-bottom));
  z-index: 90;
}

.bottom-nav__rule {
  position: absolute;
  left: 0;
  right: 0;
  top: -16px;
  height: 1px;
  background-color: $rule-strong;
}

.bottom-nav__card {
  display: flex;
  flex-direction: row;
  align-items: stretch;
  height: $nav-h;
  background-color: $card;
  border-radius: 28px;
  box-sizing: border-box;
}

.bottom-nav__item {
  flex: 1;
  min-width: $touch-min;
  min-height: $touch-min;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 3px;
}

.bottom-nav__icon {
  width: 20px;
  height: 20px;
  background-repeat: no-repeat;
  background-size: 100% 100%;
}

.bottom-nav__label {
  font-family: $font-sans;
  font-size: 12px;
  line-height: 14px;
  font-weight: 600;
  color: $ink-2;
}

.bottom-nav__item.is-active .bottom-nav__label {
  color: $accent;
}
</style>
