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
  const next: Array<{ style: Record<string, string> }> = []
  for (let i = 0; i < 18; i += 1) {
    const left = 8 + ((i * 17) % 84)
    const delay = (i % 6) * 40
    const rot = (i * 17) % 360
    next.push({
      style: {
        left: `${left}%`,
        backgroundColor: AMBER,
        animationDelay: `${delay}ms`,
        transform: `rotate(${rot}deg)`,
      },
    })
  }
  pieces.value = next
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
  top: -16px;
  width: 3px;
  height: 12px;
  border-radius: 2px;
  animation: confetti-fall 0.8s ease-out forwards;
}

@keyframes confetti-fall {
  0% {
    opacity: 1;
    transform: translateY(0) rotate(0deg);
  }
  100% {
    opacity: 0;
    transform: translateY(70vh) rotate(180deg);
  }
}
</style>
