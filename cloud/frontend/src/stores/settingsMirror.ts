/**
 * Story 6.8：设置镜像页展示缓存（非权威编辑态）。
 *
 * 裁决 A：读 GET /sync/snapshot；写 POST /sync/command；不用 /stats 拼。
 * 裁决 B：显式 CTA「保存并下发」；滑杆/分段只改 edit，不自动 POST。
 * 裁决 C：snapshot=cloud 权威；edit=编辑态。水合/提交成功 edit←权威；
 *         失败保留 edit；dirty 时后台 fetch 不覆盖 edit（只更新权威修订对）。
 * 裁决 D：每次用户新提交生成新 action_id（禁止跨次复用）。
 * 裁决 E：提交携带 base_revision=当前权威 command_revision。
 * 裁决 F：业务码 20006 采用完整快照刷新权威+edit，submitPhase=fail。
 * 裁决 G：submit 单飞 + _submitGen 丢弃过期；_authorityEpoch 防旧 GET 盖新 POST。
 * 裁决 H：本 Story 门禁以 REST 为准（onShow + 提交响应）；不消费 WS command_state。
 *
 * 禁止本地伪造已生效；禁止把 edit 当 cloud 权威写入其它页（AD-2 / AD-5）。
 * 命名：edit 对应故事编辑槽；页面/组件源码禁写禁用英文占位词以通过文案卫生扫描。
 */
import { defineStore } from 'pinia'
import {
  getSyncSnapshot,
  postSettingsCommand,
} from '../api/sync'
import { normalizeToastErrorMessage } from '../api/requestMessage'
import {
  COMMAND_STALE_REVISION_CODE,
  SETTINGS_COPY,
  SUCCESS_CODE,
} from '../utils/constants'
import type {
  BrightnessWire,
  StateSnapshot,
  TimeoutWire,
} from '../types/sync'

export type SettingsUiPhase = 'loading' | 'ready' | 'fail'
export type SettingsSubmitPhase = 'idle' | 'busy' | 'ok' | 'fail'
export type ApplyStatus = 'applied' | 'pending'

export interface SettingsEdit {
  volume: number
  brightness: BrightnessWire
  timeout: TimeoutWire
}

/** 契约 §9.1 默认：音量 50 / 亮度 mid / 熄屏 15。 */
export const DEFAULT_SETTINGS_EDIT: SettingsEdit = {
  volume: 50,
  brightness: 'mid',
  timeout: 15,
}

export type FetchSettingsReason = 'show' | 'pull' | 'retry'

function cloneSnapshot(data: StateSnapshot): StateSnapshot {
  return {
    device_id: data.device_id,
    acked_total: data.acked_total,
    local_total: data.local_total,
    round_id: data.round_id,
    round_state: data.round_state,
    round_cursor: data.round_cursor,
    pending_completion: data.pending_completion,
    command_revision: data.command_revision,
    applied_revision: data.applied_revision,
    snapshot_seq: data.snapshot_seq,
    battery_percent: data.battery_percent,
    network_mode: data.network_mode,
    audio_config_version: data.audio_config_version,
    firmware_version: data.firmware_version,
    volume: data.volume,
    brightness: data.brightness,
    timeout: data.timeout,
  }
}

/** 与 deviceStatus.commandStatus 同构：applied >= command → 已生效。 */
export function deriveApplyStatus(
  commandRevision: number,
  appliedRevision: number,
): ApplyStatus {
  return appliedRevision >= commandRevision ? 'applied' : 'pending'
}

export function clampVolume(raw: unknown): number {
  const n = Number(raw)
  if (!Number.isFinite(n)) {
    return DEFAULT_SETTINGS_EDIT.volume
  }
  return Math.min(100, Math.max(0, Math.trunc(n)))
}

export function normalizeBrightness(raw: unknown): BrightnessWire {
  if (raw === 'low' || raw === 'mid' || raw === 'high') {
    return raw
  }
  return DEFAULT_SETTINGS_EDIT.brightness
}

export function normalizeTimeout(raw: unknown): TimeoutWire {
  const n = Number(raw)
  if (n === 5 || n === 15 || n === 30) {
    return n
  }
  return DEFAULT_SETTINGS_EDIT.timeout
}

export function editFromSnapshot(snap: StateSnapshot): SettingsEdit {
  return {
    volume: clampVolume(snap.volume),
    brightness: normalizeBrightness(snap.brightness),
    timeout: normalizeTimeout(snap.timeout),
  }
}

export function isValidEdit(edit: SettingsEdit): boolean {
  return (
    Number.isInteger(edit.volume)
    && edit.volume >= 0
    && edit.volume <= 100
    && (edit.brightness === 'low'
      || edit.brightness === 'mid'
      || edit.brightness === 'high')
    && (edit.timeout === 5 || edit.timeout === 15 || edit.timeout === 30)
  )
}

/** 每次新提交新 UUID；运行时缺失 crypto.randomUUID 时用可测 fallback。 */
export function newSettingsActionId(): string {
  const c = globalThis.crypto as Crypto | undefined
  if (c && typeof c.randomUUID === 'function') {
    return c.randomUUID()
  }
  return `ewf-cmd-${Date.now()}-${Math.random().toString(36).slice(2, 10)}`
}

function editsEqual(a: SettingsEdit, b: SettingsEdit): boolean {
  return (
    a.volume === b.volume
    && a.brightness === b.brightness
    && a.timeout === b.timeout
  )
}

export const useSettingsMirrorStore = defineStore('settingsMirror', {
  state: () => ({
    uiPhase: 'loading' as SettingsUiPhase,
    snapshot: null as StateSnapshot | null,
    edit: { ...DEFAULT_SETTINGS_EDIT } as SettingsEdit,
    applyStatus: 'pending' as ApplyStatus,
    submitPhase: 'idle' as SettingsSubmitPhase,
    lastError: '' as string,
    refreshing: false,
    _fetchInflight: null as Promise<void> | null,
    _submitInflight: null as Promise<void> | null,
    _fetchGen: 0,
    _submitGen: 0,
    /** 成功提交后抬升，使在途旧 GET 不得盖写权威。 */
    _authorityEpoch: 0,
  }),
  getters: {
    dirty(state): boolean {
      if (!state.snapshot) {
        return !editsEqual(state.edit, DEFAULT_SETTINGS_EDIT)
      }
      return !editsEqual(state.edit, editFromSnapshot(state.snapshot))
    },
    volumeDisplay(state): string {
      return String(state.edit.volume)
    },
    brightnessLabel(state): string {
      if (state.edit.brightness === 'low') {
        return SETTINGS_COPY.brightnessLow
      }
      if (state.edit.brightness === 'high') {
        return SETTINGS_COPY.brightnessHigh
      }
      return SETTINGS_COPY.brightnessMid
    },
    timeoutLabel(state): string {
      if (state.edit.timeout === 5) {
        return SETTINGS_COPY.timeout5
      }
      if (state.edit.timeout === 30) {
        return SETTINGS_COPY.timeout30
      }
      return SETTINGS_COPY.timeout15
    },
    applyStatusDisplay(state): string {
      return state.applyStatus === 'applied'
        ? SETTINGS_COPY.applyApplied
        : SETTINGS_COPY.applyPending
    },
    headerCapsule(): string {
      return SETTINGS_COPY.headerCapsule
    },
    primaryCtaLabel(state): string {
      if (state.submitPhase === 'busy') {
        return SETTINGS_COPY.ctaBusy
      }
      if (state.submitPhase === 'fail') {
        return SETTINGS_COPY.failRetry
      }
      return SETTINGS_COPY.primaryCta
    },
  },
  actions: {
    reset(): void {
      this._fetchGen += 1
      this._submitGen += 1
      this._authorityEpoch += 1
      this.uiPhase = 'loading'
      this.snapshot = null
      this.edit = { ...DEFAULT_SETTINGS_EDIT }
      this.applyStatus = 'pending'
      this.submitPhase = 'idle'
      this.lastError = ''
      this.refreshing = false
      this._fetchInflight = null
      this._submitInflight = null
    },

    setVolume(value: number): void {
      this.edit = {
        ...this.edit,
        volume: clampVolume(value),
      }
    },

    setBrightness(value: BrightnessWire): void {
      this.edit = {
        ...this.edit,
        brightness: normalizeBrightness(value),
      }
    },

    setTimeoutSeconds(value: TimeoutWire): void {
      this.edit = {
        ...this.edit,
        timeout: normalizeTimeout(value),
      }
    },

    /**
     * 以快照收敛权威；syncEdit=true 时同步 edit（水合/提交成功/20006）。
     * 禁止本地抬升 applied 伪造已生效。
     */
    hydrateFromSnapshot(
      data: StateSnapshot,
      opts: { syncEdit?: boolean } = {},
    ): void {
      const syncEdit = opts.syncEdit !== false
      this.snapshot = cloneSnapshot(data)
      this.applyStatus = deriveApplyStatus(
        data.command_revision,
        data.applied_revision,
      )
      if (syncEdit) {
        this.edit = editFromSnapshot(data)
      }
      this.uiPhase = 'ready'
      this.lastError = ''
    },

    async fetchSnapshot(
      opts: { reason: FetchSettingsReason } = { reason: 'show' },
    ): Promise<{ joined: boolean }> {
      if (this._fetchInflight) {
        const epochWhenJoined = this._authorityEpoch
        await this._fetchInflight
        // join 期间若 POST 抬升权威，旧 GET 可能被丢弃——再拉一次收敛
        if (this._authorityEpoch !== epochWhenJoined) {
          return this.fetchSnapshot(opts)
        }
        return { joined: true }
      }

      const retain = this.snapshot !== null
      if (opts.reason === 'pull') {
        this.refreshing = true
      } else if (!retain) {
        this.uiPhase = 'loading'
      }
      this.lastError = ''

      const gen = this._fetchGen
      const epochAtStart = this._authorityEpoch
      const pending = this._doFetch(retain, gen, epochAtStart)
      this._fetchInflight = pending
      try {
        await pending
        return { joined: false }
      } finally {
        if (this._fetchInflight === pending) {
          this._fetchInflight = null
        }
        if (gen === this._fetchGen) {
          this.refreshing = false
        }
      }
    },

    async _doFetch(
      retain: boolean,
      gen: number,
      epochAtStart: number,
    ): Promise<void> {
      try {
        const body = await getSyncSnapshot()
        if (gen !== this._fetchGen) {
          return
        }
        if (epochAtStart !== this._authorityEpoch) {
          // 裁决 G：旧 GET 不得盖写更旧权威；若 seq 更高则仍收敛（设备已应用等）
          if (
            body.code === SUCCESS_CODE
            && body.data
            && body.data.snapshot_seq > (this.snapshot?.snapshot_seq ?? -1)
          ) {
            const wasDirty = this.dirty
            this.hydrateFromSnapshot(body.data, { syncEdit: !wasDirty })
            this._clearSubmitPhaseAfterFetch()
          }
          return
        }
        if (body.code !== SUCCESS_CODE || !body.data) {
          this.lastError =
            normalizeToastErrorMessage(body.message) || SETTINGS_COPY.failTitle
          this.uiPhase = 'fail'
          if (!retain) {
            this.snapshot = null
          }
          return
        }
        const wasDirty = this.dirty
        // dirty 时只更新权威修订对与镜像字段，不覆盖用户 edit（裁决 C）
        this.hydrateFromSnapshot(body.data, { syncEdit: !wasDirty })
        this._clearSubmitPhaseAfterFetch()
      } catch (err) {
        if (gen !== this._fetchGen) {
          return
        }
        if (epochAtStart !== this._authorityEpoch) {
          return
        }
        const msg = err instanceof Error ? err.message : String(err)
        this.lastError =
          normalizeToastErrorMessage(msg) || SETTINGS_COPY.failTitle
        this.uiPhase = 'fail'
        if (!retain) {
          this.snapshot = null
        }
      }
    },

    /** 成功拉取后收敛 CTA；busy 中不打断在途提交。 */
    _clearSubmitPhaseAfterFetch(): void {
      if (this.submitPhase === 'fail' || this.submitPhase === 'ok') {
        this.submitPhase = 'idle'
      }
    },

    /**
     * 提交 edit。busy 时 join 在途；校验失败不发请求。
     * @returns joined=true 表示并入既有 in-flight。
     */
    async submitEdit(): Promise<{ joined: boolean; ok: boolean }> {
      if (this._submitInflight) {
        await this._submitInflight
        return { joined: true, ok: this.submitPhase === 'ok' }
      }

      if (!isValidEdit(this.edit)) {
        this.submitPhase = 'fail'
        this.lastError = SETTINGS_COPY.failRetry
        return { joined: false, ok: false }
      }

      this.submitPhase = 'busy'
      this.lastError = ''
      const gen = ++this._submitGen
      const pending = this._doSubmit(gen)
      this._submitInflight = pending
      try {
        await pending
        return { joined: false, ok: this.submitPhase === 'ok' }
      } finally {
        if (this._submitInflight === pending) {
          this._submitInflight = null
        }
      }
    },

    async _doSubmit(gen: number): Promise<void> {
      const actionId = newSettingsActionId()
      // 捕获提交瞬间的 edit；在途改稿时成功回包不得盖回（AC4）
      const submitted: SettingsEdit = {
        volume: this.edit.volume,
        brightness: this.edit.brightness,
        timeout: this.edit.timeout,
      }
      const body = {
        volume: submitted.volume,
        brightness: submitted.brightness,
        timeout: submitted.timeout,
        action_id: actionId,
        ...(this.snapshot != null
          ? { base_revision: this.snapshot.command_revision }
          : {}),
      }

      try {
        const res = await postSettingsCommand(body)
        if (gen !== this._submitGen) {
          return
        }

        // 裁决 F：20006 采用完整快照（含无 data 时仍给 stale 短句）
        if (res.code === COMMAND_STALE_REVISION_CODE) {
          if (res.data) {
            this._authorityEpoch += 1
            this.hydrateFromSnapshot(res.data, { syncEdit: true })
            this.submitPhase = 'fail'
            this.lastError = SETTINGS_COPY.staleHint
            return
          }
          this.submitPhase = 'fail'
          this.lastError = SETTINGS_COPY.staleHint
          return
        }

        if (res.code !== SUCCESS_CODE || !res.data) {
          this.submitPhase = 'fail'
          this.lastError =
            normalizeToastErrorMessage(res.message) || SETTINGS_COPY.failRetry
          // 裁决：失败保留 edit，不得用失败响应盖写
          return
        }

        this._authorityEpoch += 1
        // 在途未改稿 → 同步权威；已改稿 → 只更新权威，保留最新 edit
        this.hydrateFromSnapshot(res.data, {
          syncEdit: editsEqual(this.edit, submitted),
        })
        this.submitPhase = 'ok'
        this.lastError = ''
      } catch (err) {
        if (gen !== this._submitGen) {
          return
        }
        const msg = err instanceof Error ? err.message : String(err)
        this.submitPhase = 'fail'
        this.lastError =
          normalizeToastErrorMessage(msg) || SETTINGS_COPY.failRetry
        // 离线失败保留 edit；applyStatus 保持既有（可能仍 pending）
      }
    },
  },
})
