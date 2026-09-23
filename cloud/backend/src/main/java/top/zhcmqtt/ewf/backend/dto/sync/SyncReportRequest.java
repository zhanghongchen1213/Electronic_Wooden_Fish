package top.zhcmqtt.ewf.backend.dto.sync;

import com.fasterxml.jackson.annotation.JsonProperty;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;

/**
 * 设备高水位上报请求体（契约 §6.1 / 注册表样例 {@code https_sync_report.required}）。
 *
 * <p><b>字段闭包：</b>恰为 14 个 wire 字段，逐字取自契约注册表；用 {@link JsonProperty} 锁定
 * snake_case，不得新增契约未授权字段（含 {@code delta}/{@code high_watermark}/{@code snapshot_seq}）。
 *
 * <p><b>身份：</b>{@code device_id} 在请求体中一律不可信；写路径身份只取
 * {@code UserContext.currentDeviceId()}。本字段仅满足契约必填闭包，服务层不得据此鉴权。
 */
public record SyncReportRequest(
        @JsonProperty("device_id") @NotBlank(message = "device_id 不能为空") String deviceId,
        @JsonProperty("scripture_version") @NotBlank(message = "scripture_version 不能为空") String scriptureVersion,
        @JsonProperty("local_total") @NotNull(message = "local_total 不能为空") @Min(value = 0, message = "local_total 不可为负") Integer localTotal,
        @JsonProperty("acked_total") @NotNull(message = "acked_total 不能为空") @Min(value = 0, message = "acked_total 不可为负") Integer ackedTotal,
        @JsonProperty("round_id") @NotNull(message = "round_id 不能为空") @Min(value = 1, message = "round_id 须 ≥ 1") Integer roundId,
        @JsonProperty("round_state")
        @NotBlank(message = "round_state 不能为空")
        @Pattern(regexp = "in_progress|completed", message = "round_state 取值非法") String roundState,
        @JsonProperty("round_cursor") @NotNull(message = "round_cursor 不能为空") @Min(value = 0, message = "round_cursor 不可为负") Integer roundCursor,
        @JsonProperty("pending_completion") @NotNull(message = "pending_completion 不能为空") Boolean pendingCompletion,
        @JsonProperty("applied_revision") @NotNull(message = "applied_revision 不能为空") @Min(value = 0, message = "applied_revision 不可为负") Integer appliedRevision,
        @JsonProperty("battery_percent")
        @NotNull(message = "battery_percent 不能为空")
        @Min(value = 0, message = "battery_percent 不可为负")
        @Max(value = 100, message = "battery_percent 须 ≤ 100") Integer batteryPercent,
        @JsonProperty("network_mode")
        @NotBlank(message = "network_mode 不能为空")
        @Pattern(regexp = "connected|no_signal|disabled", message = "network_mode 取值非法") String networkMode,
        @JsonProperty("audio_config_version") @NotNull(message = "audio_config_version 不能为空") @Min(value = 0, message = "audio_config_version 不可为负") Integer audioConfigVersion,
        @JsonProperty("firmware_version") @NotNull(message = "firmware_version 不能为 null") String firmwareVersion,
        @JsonProperty("action_id") @NotBlank(message = "action_id 不能为空") String actionId) {
}
