/**
 * Story 5.6 裁决 F + Story 6.4 裁决 E：本地 storage 白名单。
 * 键前缀 ewf_；契约 §11.1 frontend 三水位；禁止任何 session_key 键。
 * 2026-10-02 裁决：完全自用测试，token 三键与鉴权存取已移除。
 */

export const STORAGE_KEYS = {
  /** 契约 §11.1：最近一次查询冻结的权威水位。 */
  SNAPSHOT_SEQ: 'ewf_snapshot_seq',
  /** 裁决 A：本地已展示步进镜像（非 cloud 权威）。 */
  REPLAY_CURSOR: 'ewf_replay_cursor',
  /** 已消费的最大帧序号。 */
  LAST_APPLIED_SEQ: 'ewf_last_applied_seq',
} as const

type UniStorage = {
  getStorageSync: (key: string) => unknown
  setStorageSync: (key: string, data: unknown) => void
  removeStorageSync: (key: string) => void
}

let storageApi: UniStorage | null = null

/** 测试可注入；生产由 bindUniStorage(uni) 绑定。 */
export function bindUniStorage(api: UniStorage): void {
  storageApi = api
}

function uni(): UniStorage {
  if (storageApi) {
    return storageApi
  }
  const g = globalThis as { uni?: UniStorage }
  if (g.uni) {
    return g.uni
  }
  throw new Error('uni storage 未绑定')
}

export function getStorage<T>(key: string): T | null {
  try {
    const raw = uni().getStorageSync(key)
    if (raw === '' || raw === undefined || raw === null) {
      return null
    }
    return raw as T
  } catch {
    return null
  }
}

export function setStorage(key: string, value: unknown): void {
  try {
    uni().setStorageSync(key, value)
  } catch {
    // storage 不可用时不崩页面
  }
}

export function removeStorage(key: string): void {
  try {
    uni().removeStorageSync(key)
  } catch {
    // ignore
  }
}

function readNonNegInt(key: string): number | null {
  const raw = getStorage<unknown>(key)
  if (raw === null || raw === undefined || raw === '') {
    return null
  }
  const n = typeof raw === 'number' ? raw : Number(raw)
  if (!Number.isFinite(n) || n < 0) {
    return null
  }
  return Math.floor(n)
}

export function getPersistedSnapshotSeq(): number | null {
  return readNonNegInt(STORAGE_KEYS.SNAPSHOT_SEQ)
}

export function getPersistedReplayCursor(): number | null {
  return readNonNegInt(STORAGE_KEYS.REPLAY_CURSOR)
}

export function getPersistedLastAppliedSeq(): number | null {
  return readNonNegInt(STORAGE_KEYS.LAST_APPLIED_SEQ)
}

/** 裁决 E：成功态水位写入；登出/reset 必须清。 */
export function persistReadingWatermarks(input: {
  snapshotSeq: number
  replayCursor: number
  lastAppliedSeq: number
}): void {
  setStorage(STORAGE_KEYS.SNAPSHOT_SEQ, Math.max(0, Math.floor(input.snapshotSeq)))
  setStorage(STORAGE_KEYS.REPLAY_CURSOR, Math.max(0, Math.floor(input.replayCursor)))
  setStorage(STORAGE_KEYS.LAST_APPLIED_SEQ, Math.max(0, Math.floor(input.lastAppliedSeq)))
}

export function clearReadingWatermarks(): void {
  removeStorage(STORAGE_KEYS.SNAPSHOT_SEQ)
  removeStorage(STORAGE_KEYS.REPLAY_CURSOR)
  removeStorage(STORAGE_KEYS.LAST_APPLIED_SEQ)
}

/** 红队：禁止把 session_key 写入任何键。 */
export function assertNoSessionKeyStorage(): void {
  const forbidden = ['session_key', 'sessionKey', 'ewf_session_key']
  for (const key of forbidden) {
    if (getStorage(key) != null) {
      throw new Error('禁止持久化 session_key')
    }
  }
}
