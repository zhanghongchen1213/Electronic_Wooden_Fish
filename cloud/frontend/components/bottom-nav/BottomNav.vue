<template>
  <!--
    Story 6.1 裁决 B：优先 pages.json 原生 tabBar（阅读/记录/设备/设置，
    selectedColor=#a66b3a）。本组件保留为自定义底栏备选，触区 ≥44×44。
    登录页不进 tabBar。
  -->
  <view class="bottom-nav" role="navigation">
    <view
      v-for="item in items"
      :key="item.key"
      class="bottom-nav__item"
      :class="{ 'is-active': item.key === current }"
      @click="onSelect(item.key)"
    >
      <text class="bottom-nav__label">{{ item.label }}</text>
    </view>
  </view>
</template>

<script setup lang="ts">
import { NAV_TAB_KEYS, NAV_TAB_LABELS, NAV_TAB_ROUTES, type PrimaryTabKey } from '../../utils/constants'
import { useAppShellStore } from '../../stores/appShell'

const props = defineProps<{
  current: PrimaryTabKey
}>()

const items = NAV_TAB_KEYS.map((key, index) => ({
  key,
  label: NAV_TAB_LABELS[index],
}))

const shell = useAppShellStore()

function onSelect(key: PrimaryTabKey) {
  if (key === props.current) {
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
  left: 0;
  right: 0;
  bottom: 0;
  height: $nav-h;
  display: flex;
  flex-direction: row;
  align-items: stretch;
  background-color: $card;
  border-top: 1px solid $divider;
  box-sizing: border-box;
  padding-bottom: env(safe-area-inset-bottom);
}

.bottom-nav__item {
  flex: 1;
  min-width: $touch-min;
  min-height: $touch-min;
  display: flex;
  align-items: center;
  justify-content: center;
}

.bottom-nav__label {
  font-size: 13px;
  color: $ink-3;
}

.bottom-nav__item.is-active .bottom-nav__label {
  color: $accent;
  font-weight: 500;
}
</style>
