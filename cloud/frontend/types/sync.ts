/**
 * Story 6.3：同步快照与 WS 帧类型。
 *
 * 裁决：字段名与 wire 一一对应（snake_case），对齐契约 §6.2 / §10 与 backend
 * StateSnapshotResponse 17 字段。禁止增加契约未授权的展示字形 wire 字段。
 * 「当前字」由 round_cursor + canonical STEPS 派生，不是帧字段。
 */

/** 契约 contract_version；不一致帧拒绝。 */
export const SYNC_CONTRACT_VERSION = 'SC-1.0.0' as const

/**
 * GET /sync/snapshot 的 data 载荷（17 字段闭包）。
 * 差量不是字段：派生量 = acked_total − 请求基线。
 */
export interface StateSnapshot {
  device_id: string
  acked_total: number
  local_total: number
  round_id: number
  round_state: string
  round_cursor: number
  pending_completion: boolean
  command_revision: number
  applied_revision: number
  snapshot_seq: number
  battery_percent: number
  network_mode: string
  audio_config_version: number
  firmware_version: string
  volume: number
  brightness: string
  timeout: number
}

/** 可选查询声明；仅作基线提示，不发明新参数名。 */
export interface SnapshotQuery {
  acked_total?: number
  snapshot_seq?: number
}

export type WsFrameType =
  | 'snapshot'
  | 'delta'
  | 'completion'
  | 'command_state'
  | 'heartbeat'
  | 'heartbeat_ack'
  | 'error'

/** 每帧必带 contract_version / type / seq。 */
export interface WsFrameBase {
  contract_version: string
  type: WsFrameType
  seq: number
}

/** delta 载荷仅 acked_total/local_total/round_id/round_cursor（无字形）。 */
export interface WsDeltaFrame extends WsFrameBase {
  type: 'delta'
  acked_total: number
  local_total: number
  round_id: number
  round_cursor: number
}

export interface WsHeartbeatFrame extends WsFrameBase {
  type: 'heartbeat'
}

export interface WsHeartbeatAckFrame extends WsFrameBase {
  type: 'heartbeat_ack'
}

export interface WsCompletionFrame extends WsFrameBase {
  type: 'completion'
  round_id: number
  round_state: string
  pending_completion: boolean
}

export interface WsErrorFrame extends WsFrameBase {
  type: 'error'
  error: {
    code: string
    message: string
  }
}

export type WsInboundFrame =
  | WsDeltaFrame
  | WsHeartbeatFrame
  | WsCompletionFrame
  | WsErrorFrame
  | (WsFrameBase & Record<string, unknown>)

/**
 * Story 6.5：POST /sync/round-action 请求体（仅两字段；契约 §7/§9）。
 * 禁止发明额外 wire 字段。
 */
export type RoundActionKind = 'restart' | 'exit'

export interface RoundActionRequest {
  action: RoundActionKind
  action_id: string
}

/**
 * Story 6.6：GET /sync/stats 的 data 载荷（与 5.3 HistoryStatsResponse 对齐）。
 * 裁决 A：仅消费本 DTO；禁止从 snapshot/WS/readingStream 拼统计。
 * wire 全部 snake_case；不得发明字段名以外的统计键。
 */
export interface HistoryStats {
  today_taps: number
  last_7_days_taps: number
  last_30_days_taps: number
  total_taps: number
  streak_days: number
  empty: boolean
  pending_sync: boolean
}

/**
 * Story 6.8：POST /sync/command 请求体（与 5.4 SettingsCommandRequest 对齐）。
 * 字段闭包：volume / brightness / timeout / action_id / 可选 base_revision。
 * 禁止客户端提交 command_revision / applied_revision 作权威写入。
 * brightness wire 仅 low|mid|high（禁止 medium）。
 */
export type BrightnessWire = 'low' | 'mid' | 'high'

export type TimeoutWire = 5 | 15 | 30

export interface SettingsCommandRequest {
  volume: number
  brightness: BrightnessWire
  timeout: TimeoutWire
  action_id: string
  base_revision?: number
}
