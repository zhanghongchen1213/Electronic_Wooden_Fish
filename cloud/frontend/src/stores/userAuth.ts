/**
 * Story 6.2 + AD-2：非权威登录态。
 *
 * - deviceId 仅内存缓存（裁决 E：不落盘，避免突破 ewf_ 三键白名单）。
 * - isLoggedIn = 有 access token 且未本地判过期（双条件；勿只看 deviceId）。
 * - 禁止写入 acked_total / 进度 / 统计；权威只在 backend。
 * - 只消费 4.4 登录 API 与 5.6 request；不重写单飞。
 */
import { defineStore } from 'pinia'
import { loginByWechatMini } from '../api/auth'
import { normalizeToastErrorMessage } from '../api/requestMessage'
import { LOGIN_COPY, SUCCESS_CODE } from '../utils/constants'
import {
  clearTokens,
  getAccessToken,
  isTokenExpired,
  saveTokens,
} from '../utils/storage'
import { useDeviceStatusStore } from './deviceStatus'
import { useReadingStreamStore } from './readingStream'
import { useRecordsStatsStore } from './recordsStats'
import { useSettingsMirrorStore } from './settingsMirror'

export type LoginPhase = 'idle' | 'loading' | 'perm_error' | 'ok'

export const useUserAuthStore = defineStore('userAuth', {
  state: () => ({
    /** 非权威：登录响应缓存的唯一设备 id；不作多设备选择键。 */
    deviceId: '' as string,
    loading: false,
    /** 权限失败一句话；颜色不是唯一通道。 */
    permErrorMessage: '' as string,
    loginPhase: 'idle' as LoginPhase,
  }),
  getters: {
    isLoggedIn(state): boolean {
      void state
      const token = getAccessToken()
      return Boolean(token) && !isTokenExpired()
    },
  },
  actions: {
    /**
     * 用 wx code 换令牌。成功写 token+deviceId；失败进 perm_error 且不写半套 token。
     */
    async loginWithWxCode(code: string): Promise<boolean> {
      this.loading = true
      this.loginPhase = 'loading'
      this.permErrorMessage = ''
      try {
        if (!code || !code.trim()) {
          this.loginPhase = 'perm_error'
          this.permErrorMessage = LOGIN_COPY.permTitle
          return false
        }
        const body = await loginByWechatMini(code.trim())
        if (
          body.code === SUCCESS_CODE
          && body.data?.accessToken
          && body.data?.refreshToken
          && body.data?.deviceId
        ) {
          saveTokens(
            body.data.accessToken,
            body.data.refreshToken,
            body.data.expiresIn ?? 7200,
          )
          this.deviceId = body.data.deviceId
          this.loginPhase = 'ok'
          this.permErrorMessage = ''
          return true
        }
        // 失败路径：不得写入半套 token
        this.loginPhase = 'perm_error'
        const cleaned = normalizeToastErrorMessage(body.message)
        this.permErrorMessage = cleaned || LOGIN_COPY.permTitle
        return false
      } catch {
        this.loginPhase = 'perm_error'
        this.permErrorMessage = LOGIN_COPY.permTitle
        return false
      } finally {
        this.loading = false
      }
    },

    /** 与登录失败路径一致：清 token + deviceId + 错误态，禁止半清。 */
    clearSession(): void {
      clearTokens()
      this.deviceId = ''
      this.permErrorMessage = ''
      this.loginPhase = 'idle'
      this.loading = false
    },

    logout(): void {
      // Story 6.3：登出关闭阅读 LIVE socket / 定时器（非权威展示态一并清）
      try {
        useReadingStreamStore().reset()
      } catch {
        // pinia 未就绪时忽略
      }
      // Story 6.6：登出清记录页展示缓存（与 6.2 登出路径一致）
      try {
        useRecordsStatsStore().reset()
      } catch {
        // pinia 未就绪时忽略
      }
      // Story 6.7：登出清设备页展示缓存
      try {
        useDeviceStatusStore().reset()
      } catch {
        // pinia 未就绪时忽略
      }
      // Story 6.8：登出清设置镜像展示缓存
      try {
        useSettingsMirrorStore().reset()
      } catch {
        // pinia 未就绪时忽略
      }
      this.clearSession()
    },
  },
})
