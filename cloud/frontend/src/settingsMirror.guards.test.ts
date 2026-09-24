/**
 * Story 6.8：设置镜像护栏。
 * ① 默认值 50/mid/15 ② 域校验 ③ 修订派生 ④ 成功后 pending
 * ⑤ 过期响应不覆盖 ⑥ 20006 采用快照 ⑦ 单飞不双发
 * ⑧ 禁用词 / 无第二 uni.request / brightness≠medium
 */
import { readFileSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { createPinia, setActivePinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import {
  COMMAND_STALE_REVISION_CODE,
  SETTINGS_COPY,
} from './utils/constants'
import {
  DEFAULT_SETTINGS_EDIT,
  deriveApplyStatus,
  isValidEdit,
  newSettingsActionId,
} from './stores/settingsMirror'
import type { StateSnapshot } from './types/sync'

const getSyncSnapshot = vi.fn()
const postSettingsCommand = vi.fn()

vi.mock('./api/sync', () => ({
  getSyncSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  refreshSettingsSnapshot: (...args: unknown[]) => getSyncSnapshot(...args),
  postSettingsCommand: (...args: unknown[]) => postSettingsCommand(...args),
  getHistoryStats: vi.fn(),
  postRoundAction: vi.fn(),
  refreshDeviceSnapshot: vi.fn(),
}))

const srcRoot = fileURLToPath(new URL('.', import.meta.url))

function makeSnapshot(partial: Partial<StateSnapshot> = {}): StateSnapshot {
  return {
    device_id: 'dev-1',
    acked_total: 10,
    local_total: 10,
    round_id: 1,
    round_state: 'active',
    round_cursor: 0,
    pending_completion: false,
    command_revision: 1,
    applied_revision: 1,
    snapshot_seq: 1,
    battery_percent: 80,
    network_mode: 'connected',
    audio_config_version: 1,
    firmware_version: '1.0.0',
    volume: 50,
    brightness: 'mid',
    timeout: 15,
    ...partial,
  }
}

describe('settingsMirror helpers (Story 6.8)', () => {
  it('默认值 50 / mid / 15', () => {
    expect(DEFAULT_SETTINGS_EDIT).toEqual({
      volume: 50,
      brightness: 'mid',
      timeout: 15,
    })
  })

  it('域校验：合法通过，非法拒绝', () => {
    expect(isValidEdit({ volume: 50, brightness: 'mid', timeout: 15 })).toBe(true)
    expect(isValidEdit({ volume: -1, brightness: 'mid', timeout: 15 })).toBe(false)
    expect(isValidEdit({ volume: 50, brightness: 'mid', timeout: 10 })).toBe(false)
    expect(
      isValidEdit({
        volume: 50,
        brightness: 'medium' as 'mid',
        timeout: 15,
      }),
    ).toBe(false)
  })

  it('修订派生：applied>=command → 已生效，否则待设备应用', () => {
    expect(deriveApplyStatus(3, 3)).toBe('applied')
    expect(deriveApplyStatus(3, 5)).toBe('applied')
    expect(deriveApplyStatus(5, 3)).toBe('pending')
  })

  it('每次新提交生成新 action_id', () => {
    const a = newSettingsActionId()
    const b = newSettingsActionId()
    expect(a).toBeTruthy()
    expect(b).toBeTruthy()
    expect(a).not.toBe(b)
  })
})

describe('settingsMirror store (Story 6.8)', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    getSyncSnapshot.mockReset()
    postSettingsCommand.mockReset()
  })

  it('水合默认镜像并显示已生效', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.uiPhase).toBe('ready')
    expect(store.edit).toEqual({ volume: 50, brightness: 'mid', timeout: 15 })
    expect(store.applyStatus).toBe('applied')
    expect(store.applyStatusDisplay).toBe(SETTINGS_COPY.applyApplied)
    expect(store.headerCapsule).toBe(SETTINGS_COPY.headerCapsule)
    expect(store.primaryCtaLabel).toBe(SETTINGS_COPY.primaryCta)
  })

  it('提交成功且 applied < command → 待设备应用', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ command_revision: 1, applied_revision: 1 }),
    })
    postSettingsCommand.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 70,
        brightness: 'high',
        timeout: 30,
        command_revision: 2,
        applied_revision: 1,
      }),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(70)
    store.setBrightness('high')
    store.setTimeoutSeconds(30)
    const result = await store.submitEdit()
    expect(result.ok).toBe(true)
    expect(store.applyStatus).toBe('pending')
    expect(store.applyStatusDisplay).toBe(SETTINGS_COPY.applyPending)
    expect(store.edit.volume).toBe(70)
    expect(store.edit.brightness).toBe('high')
    expect(store.edit.timeout).toBe(30)
    expect(postSettingsCommand).toHaveBeenCalledTimes(1)
    const body = postSettingsCommand.mock.calls[0][0]
    expect(body).toMatchObject({
      volume: 70,
      brightness: 'high',
      timeout: 30,
      base_revision: 1,
    })
    expect(body.action_id).toBeTruthy()
    expect(body).not.toHaveProperty('command_revision')
    expect(body).not.toHaveProperty('applied_revision')
  })

  it('追上 applied ≥ command → 已生效', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 70,
        command_revision: 2,
        applied_revision: 2,
      }),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.applyStatus).toBe('applied')
    expect(store.applyStatusDisplay).toBe('已生效')
  })

  it('dirty 时后台 fetch 不覆盖 edit，仍更新修订态', async () => {
    getSyncSnapshot
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeSnapshot({
          volume: 50,
          command_revision: 1,
          applied_revision: 1,
        }),
      })
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeSnapshot({
          volume: 40,
          command_revision: 3,
          applied_revision: 1,
        }),
      })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(88)
    expect(store.dirty).toBe(true)
    await store.fetchSnapshot({ reason: 'retry' })
    expect(store.edit.volume).toBe(88)
    expect(store.snapshot?.command_revision).toBe(3)
    expect(store.applyStatus).toBe('pending')
  })

  it('过期 submit 响应不覆盖更新 edit', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    let resolveFirst!: (v: unknown) => void
    const firstPromise = new Promise((resolve) => {
      resolveFirst = resolve
    })
    postSettingsCommand
      .mockImplementationOnce(() => firstPromise)
      .mockResolvedValueOnce({
        code: 0,
        message: 'ok',
        data: makeSnapshot({
          volume: 90,
          command_revision: 3,
          applied_revision: 1,
        }),
      })

    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(60)
    const p1 = store.submitEdit()
    // 不等待完成：直接抬升 gen 模拟更新提交已接手
    store._submitGen += 1
    resolveFirst({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 60,
        command_revision: 2,
        applied_revision: 1,
      }),
    })
    await p1
    // 过期响应被丢弃：edit 仍为用户当前值，未被旧成功写回异常值
    expect(store.edit.volume).toBe(60)
    // 权威未因过期响应被写成 command_revision=2（gen 已抬升）
    expect(store.snapshot?.command_revision).toBe(1)
  })

  it('20006 采用完整快照并提示可再保存', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ command_revision: 2, applied_revision: 2 }),
    })
    postSettingsCommand.mockResolvedValue({
      code: COMMAND_STALE_REVISION_CODE,
      message: 'stale',
      data: makeSnapshot({
        volume: 55,
        brightness: 'low',
        timeout: 5,
        command_revision: 4,
        applied_revision: 2,
      }),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(99)
    const result = await store.submitEdit()
    expect(result.ok).toBe(false)
    expect(store.submitPhase).toBe('fail')
    expect(store.lastError).toBe(SETTINGS_COPY.staleHint)
    expect(store.edit.volume).toBe(55)
    expect(store.edit.brightness).toBe('low')
    expect(store.snapshot?.command_revision).toBe(4)
    expect(store.applyStatus).toBe('pending')
  })

  it('单飞：并发 submit 不双发', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    let resolvePost!: (v: unknown) => void
    postSettingsCommand.mockImplementation(
      () =>
        new Promise((resolve) => {
          resolvePost = resolve
        }),
    )
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(66)
    const a = store.submitEdit()
    const b = store.submitEdit()
    resolvePost({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 66,
        command_revision: 2,
        applied_revision: 1,
      }),
    })
    await Promise.all([a, b])
    expect(postSettingsCommand).toHaveBeenCalledTimes(1)
  })

  it('传输失败保留 edit，不回滚', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ volume: 50 }),
    })
    postSettingsCommand.mockResolvedValue({
      code: 50000,
      message: 'network',
      data: null,
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(77)
    await store.submitEdit()
    expect(store.edit.volume).toBe(77)
    expect(store.submitPhase).toBe('fail')
    expect(store.snapshot?.volume).toBe(50)
  })

  it('登出 reset 清缓存', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ volume: 42 }),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.reset()
    expect(store.snapshot).toBeNull()
    expect(store.edit.volume).toBe(50)
    expect(store.uiPhase).toBe('loading')
  })

  it('COMMAND_STALE_REVISION_CODE 钉死契约字面量 20006', () => {
    expect(COMMAND_STALE_REVISION_CODE).toBe(20006)
  })

  it('首拉失败 → uiPhase=fail；已有快照后再失败 retain', async () => {
    getSyncSnapshot.mockResolvedValueOnce({
      code: 50000,
      message: 'down',
      data: null,
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    expect(store.uiPhase).toBe('fail')
    expect(store.snapshot).toBeNull()

    getSyncSnapshot.mockResolvedValueOnce({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ volume: 40, snapshot_seq: 2 }),
    })
    await store.fetchSnapshot({ reason: 'retry' })
    expect(store.uiPhase).toBe('ready')
    expect(store.snapshot?.volume).toBe(40)

    getSyncSnapshot.mockResolvedValueOnce({
      code: 50000,
      message: 'down',
      data: null,
    })
    await store.fetchSnapshot({ reason: 'pull' })
    expect(store.uiPhase).toBe('fail')
    expect(store.snapshot?.volume).toBe(40)
  })

  it('成功拉取后清除 submitPhase=fail，CTA 回到主文案', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    postSettingsCommand.mockResolvedValue({
      code: 50000,
      message: 'network',
      data: null,
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(70)
    await store.submitEdit()
    expect(store.submitPhase).toBe('fail')
    expect(store.primaryCtaLabel).toBe(SETTINGS_COPY.failRetry)

    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ snapshot_seq: 2 }),
    })
    await store.fetchSnapshot({ reason: 'pull' })
    expect(store.submitPhase).toBe('idle')
    expect(store.primaryCtaLabel).toBe(SETTINGS_COPY.primaryCta)
  })

  it('在途改稿：成功响应不覆盖更新 edit', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({ volume: 50 }),
    })
    let resolvePost!: (v: unknown) => void
    postSettingsCommand.mockImplementation(
      () =>
        new Promise((resolve) => {
          resolvePost = resolve
        }),
    )
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(66)
    const pending = store.submitEdit()
    store.setVolume(77)
    resolvePost({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 66,
        command_revision: 2,
        applied_revision: 1,
        snapshot_seq: 3,
      }),
    })
    await pending
    expect(store.edit.volume).toBe(77)
    expect(store.snapshot?.volume).toBe(66)
    expect(store.snapshot?.command_revision).toBe(2)
  })

  it('旧 GET 不得盖写更新 POST 权威（_authorityEpoch）', async () => {
    let resolveGet!: (v: unknown) => void
    getSyncSnapshot.mockImplementationOnce(
      () =>
        new Promise((resolve) => {
          resolveGet = resolve
        }),
    )
    postSettingsCommand.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 80,
        command_revision: 5,
        applied_revision: 4,
        snapshot_seq: 10,
      }),
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    // 先人工水合，避免首拉阻塞
    store.hydrateFromSnapshot(
      makeSnapshot({
        volume: 50,
        command_revision: 1,
        applied_revision: 1,
        snapshot_seq: 1,
      }),
    )
    const fetchPending = store.fetchSnapshot({ reason: 'pull' })
    store.setVolume(80)
    await store.submitEdit()
    expect(store.snapshot?.command_revision).toBe(5)

    resolveGet({
      code: 0,
      message: 'ok',
      data: makeSnapshot({
        volume: 50,
        command_revision: 1,
        applied_revision: 1,
        snapshot_seq: 1,
      }),
    })
    await fetchPending
    expect(store.snapshot?.command_revision).toBe(5)
    expect(store.snapshot?.volume).toBe(80)
  })

  it('20006 无 data 仍提示 staleHint', async () => {
    getSyncSnapshot.mockResolvedValue({
      code: 0,
      message: 'ok',
      data: makeSnapshot(),
    })
    postSettingsCommand.mockResolvedValue({
      code: 20006,
      message: 'stale',
      data: null,
    })
    const { useSettingsMirrorStore } = await import('./stores/settingsMirror')
    const store = useSettingsMirrorStore()
    await store.fetchSnapshot({ reason: 'show' })
    store.setVolume(90)
    await store.submitEdit()
    expect(store.submitPhase).toBe('fail')
    expect(store.lastError).toBe(SETTINGS_COPY.staleHint)
    expect(store.edit.volume).toBe(90)
  })
})

describe('settings source hygiene (Story 6.8)', () => {
  it('设置页与组件无禁用词；brightness 不用 medium；无第二 uni.request', () => {
    const files = [
      'pages/settings/index.vue',
      'components/settings/VolumeSlider.vue',
      'components/settings/SegmentedControl.vue',
      'components/settings/ApplyStatusBar.vue',
    ]
    for (const rel of files) {
      const text = readFileSync(join(srcRoot, rel), 'utf8')
      expect(text).not.toMatch(/\bTODO\b/)
      expect(text).not.toMatch(/\bdraft\b/i)
      expect(text).not.toMatch(/\bplaceholder\b/i)
      expect(text).not.toMatch(/data-pencil-id/)
      expect(text).not.toMatch(/\buni\.request\b/)
      expect(text).not.toMatch(/\bmedium\b/)
    }
    const storeSrc = readFileSync(join(srcRoot, 'stores/settingsMirror.ts'), 'utf8')
    expect(storeSrc).not.toMatch(/\buni\.request\b/)
    expect(storeSrc).not.toMatch(/\bmedium\b/)
    const apiSrc = readFileSync(join(srcRoot, 'api/sync.ts'), 'utf8')
    expect(apiSrc).toContain('postSettingsCommand')
    expect(apiSrc).toContain('/sync/command')
    expect(SETTINGS_COPY.primaryCta).toBe('保存并下发')
    expect(SETTINGS_COPY.applyPending).toBe('待设备应用')
    expect(SETTINGS_COPY.brightnessMid).toBe('中')
  })
})
