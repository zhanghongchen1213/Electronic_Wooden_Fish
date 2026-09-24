/**
 * Story 6.3 + 6.5 + 6.6 + 6.7 + 6.8：同步快照 / 跨轮动作 / 历史统计 / 设置命令薄封装。
 * 唯一 HTTP 出口仍是 request.get/post；禁止第二请求栈。
 * 只消费 4.6 GET /sync/snapshot、5.2 POST /sync/round-action、5.3 GET /sync/stats、
 * 5.4 POST /sync/command；不改 backend、不改契约字节。不实现 sync_now 写路径。
 *
 * Story 6.6 裁决 A：统计仅走 getHistoryStats → GET /sync/stats；
 * 不用 snapshot 拼、不用 WS 推。
 * Story 6.7 裁决 A：设备页仅 getSyncSnapshot / refreshDeviceSnapshot 别名。
 * Story 6.8 裁决 A：设置页读 snapshot、写 postSettingsCommand；不用 /stats 拼设置。
 */
import type { ApiResponse } from '../types/api'
import type {
  HistoryStats,
  RoundActionRequest,
  SettingsCommandRequest,
  SnapshotQuery,
  StateSnapshot,
} from '../types/sync'
import { get, post } from './request'

/**
 * 拉取权威快照。可选 acked_total / snapshot_seq 仅作声明，不发明新参数名。
 */
export function getSyncSnapshot(
  params?: SnapshotQuery,
): Promise<ApiResponse<StateSnapshot>> {
  return get<StateSnapshot>('/sync/snapshot', {
    data: params,
    showError: false,
  })
}

/**
 * Story 6.7：设备页刷新别名；仍走 getSyncSnapshot → request.get('/sync/snapshot')。
 * 不新增 wire 参数；不调用不存在的 sync_now。
 */
export function refreshDeviceSnapshot(
  params?: SnapshotQuery,
): Promise<ApiResponse<StateSnapshot>> {
  return getSyncSnapshot(params)
}

/**
 * Story 6.5：跨轮动作（restart|exit）。body 仅 { action, action_id }；
 * 响应 data 为 17 字段快照。幂等由 backend action_id 保证。
 */
export function postRoundAction(
  body: RoundActionRequest,
): Promise<ApiResponse<StateSnapshot>> {
  return post<StateSnapshot>('/sync/round-action', body, {
    showError: false,
  })
}

/**
 * Story 6.6：拉取已确认历史统计。查询只读，不触发 catch-up。
 */
export function getHistoryStats(): Promise<ApiResponse<HistoryStats>> {
  return get<HistoryStats>('/sync/stats', {
    showError: false,
  })
}

/**
 * Story 6.8：设置命令下发。body 仅 volume/brightness/timeout/action_id/可选 base_revision；
 * 响应 data 为 17 字段快照。幂等由 backend action_id 保证；不改契约字节。
 */
export function postSettingsCommand(
  body: SettingsCommandRequest,
): Promise<ApiResponse<StateSnapshot>> {
  return post<StateSnapshot>('/sync/command', body, {
    showError: false,
  })
}

/**
 * Story 6.8：设置页刷新别名；仍走 getSyncSnapshot → request.get('/sync/snapshot')。
 */
export function refreshSettingsSnapshot(
  params?: SnapshotQuery,
): Promise<ApiResponse<StateSnapshot>> {
  return getSyncSnapshot(params)
}
