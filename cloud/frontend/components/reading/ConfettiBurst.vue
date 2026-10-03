<template>
  <view
    v-if="active"
    class="confetti"
    aria-hidden="true"
  >
    <view
      v-for="(p, i) in pieces"
      :key="i"
      class="confetti__piece"
      :style="p.style"
    />
  </view>
</template>

<script setup lang="ts">
/**
 * Story 6.5 裁决 F：短促礼花（CSS 粒子，琥珀 #e6bd69）；默认 ≤1s 结束。
 * 减少动效时跳过粒子，仍由父级保留一次完成确认（弹窗）。
 */
import { onUnmounted, ref, watch } from 'vue'

const props = defineProps<{
  pending: boolean
}>()

const emit = defineEmits<{
  done: []
}>()

const AMBER = '#e6bd69'
const DURATION_MS = 800

const active = ref(false)
const pieces = ref<Array<{ style: Record<string, string> }>>([])
let timer: ReturnType<typeof setTimeout> | null = null

function prefersReducedMotion(): boolean {
  try {
    const g = globalThis as {
      uni?: { getSystemInfoSync?: () => { reduceMotion?: boolean; platform?: string } }
      matchMedia?: (q: string) => { matches: boolean }
    }
    if (g.matchMedia && g.matchMedia('(prefers-reduced-motion: reduce)').matches) {
      return true
    }
    const info = g.uni?.getSystemInfoSync?.()
    if (info && info.reduceMotion === true) {
      return true
    }
  } catch {
    // ignore
  }
  return false
}

function clearTimer(): void {
  if (timer != null) {
    clearTimeout(timer)
    timer = null
  }
}

function finish(): void {
  clearTimer()
  active.value = false
  pieces.value = []
  emit('done')
}

function startBurst(): void {
  clearTimer()
  if (prefersReducedMotion()) {
    active.value = false
    pieces.value = []
    emit('done')
    return
  }
  // 7 条金彩带，尺寸与位置对拍导出稿 OVERLAY.DONE confetti 节点（390 基线）
  const RIBBONS = [
    { x: 50, y: 86, w: 3, h: 18 },
    { x: 108, y: 60, w: 2, h: 12 },
    { x: 176, y: 96, w: 3, h: 16 },
    { x: 250, y: 72, w: 2, h: 12 },
    { x: 318, y: 112, w: 3, h: 18 },
    { x: 76, y: 154, w: 2, h: 10 },
    { x: 286, y: 168, w: 2, h: 14 },
  ]
  pieces.value = RIBBONS.map((r, i) => ({
    style: {
      left: `${r.x}px`,
      top: `${r.y}px`,
      width: `${r.w}px`,
      height: `${r.h}px`,
      backgroundColor: AMBER,
      animationDelay: `${i * 40}ms`,
    },
  }))
  active.value = true
  timer = setTimeout(finish, DURATION_MS)
}

watch(
  () => props.pending,
  (v) => {
    if (v) {
      startBurst()
    }
  },
  { immediate: true },
)

onUnmounted(() => {
  clearTimer()
  if (active.value || pieces.value.length > 0) {
    emit('done')
  }
})
</script>

<style lang="scss" scoped>
.confetti {
  position: fixed;
  left: 0;
  top: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
  z-index: 110;
  overflow: hidden;
}

.confetti__piece {
  position: absolute;
  border-radius: 999px;
  animation: confetti-fall 0.8s ease-out forwards;
}

@keyframes confetti-fall {
  0% {
    opacity: 1;
    transform: translateY(0) rotate(12deg);
  }
  100% {
    opacity: 0;
    transform: translateY(70vh) rotate(192deg);
  }
}
</style>
