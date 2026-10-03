<template>
  <!--
    Story 6.9 裁决 F：印谱签名层（对拍 HTML style-signature / hero-seal）。
    装饰不得承载正式进度/统计数字；禁止把设计导出属性拷进生产 DOM。
    阅读页 17 槽基线由 ReadingLine 保留，本组件不另造刻度条。
  -->
  <view
    class="sig"
    :class="[`sig--${variant}`]"
    aria-hidden="true"
  >
    <text
      v-if="showFolio"
      class="sig__folio"
    >001</text>

    <view
      v-if="showRule"
      class="sig__rule"
    />

    <view class="sig__seal">
      <view class="sig__seal-inner">
        <text class="sig__seal-label">印{'\n'}谱</text>
      </view>
    </view>
  </view>
</template>

<script setup lang="ts">
/**
 * 单一印谱组件 + 页级 variant（login|reading|records|device|settings|overlay）。
 * HTML 参照：ewf-miniapp-ui-export.html 各页 [CMP:style-signature]。
 * login = hero-seal（64 方角 + folio「001」）；overlay = 70 方角章。
 */
import { computed } from 'vue'

const props = withDefaults(
  defineProps<{
    variant?: 'login' | 'reading' | 'records' | 'device' | 'settings' | 'overlay'
  }>(),
  { variant: 'records' },
)

const showFolio = computed(() => props.variant === 'login')
const showRule = computed(
  () => props.variant !== 'login' && props.variant !== 'overlay',
)
</script>

<style lang="scss" scoped>
@import '../../styles/tokens.scss';

.sig {
  position: relative;
  pointer-events: none;
  box-sizing: border-box;
}

.sig--login {
  width: 64px;
  height: 92px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 8px;
}

.sig--reading,
.sig--records,
.sig--device,
.sig--settings {
  width: 100%;
  min-height: 72px;
  margin-top: 12px;
  display: flex;
  flex-direction: column;
  align-items: flex-end;
}

.sig--overlay {
  width: 70px;
  height: 70px;
  margin: 0 auto 28px;
}

.sig__folio {
  color: $ink-2;
  font-size: 11px;
  font-weight: 600;
  line-height: 13px;
  order: 2;
}

.sig--login .sig__seal {
  order: 1;
}

.sig__rule {
  height: 1px;
  width: 100%;
  background-color: $divider;
  margin-bottom: 10px;
  align-self: stretch;
}

.sig__seal {
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: rgba(166, 76, 62, 0.09);
  outline: 1px solid #a64c3e;
  outline-offset: -0.5px;
  border-radius: 4px;
}

.sig--login .sig__seal {
  width: 64px;
  height: 64px;
}

.sig--reading .sig__seal,
.sig--records .sig__seal,
.sig--device .sig__seal,
.sig--settings .sig__seal {
  width: 58px;
  height: 58px;
}

.sig--overlay .sig__seal {
  width: 70px;
  height: 70px;
}

.sig__seal-inner {
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  outline: 1px solid #a64c3e;
  outline-offset: -0.5px;
  border-radius: 2px;
}

.sig--login .sig__seal-inner {
  width: 52px;
  height: 52px;
}

.sig--reading .sig__seal-inner,
.sig--records .sig__seal-inner,
.sig--device .sig__seal-inner,
.sig--settings .sig__seal-inner {
  width: 46px;
  height: 46px;
}

.sig--overlay .sig__seal-inner {
  width: 58px;
  height: 58px;
}

.sig__seal-label {
  color: #a64c3e;
  font-weight: 600;
  text-align: center;
  line-height: 1.15;
  white-space: pre-line;
  font-size: 15px;
}

.sig--login .sig__seal-label,
.sig--overlay .sig__seal-label {
  font-size: 17px;
  line-height: 20px;
}
</style>
