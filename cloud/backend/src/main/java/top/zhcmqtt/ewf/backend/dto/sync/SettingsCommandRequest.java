package top.zhcmqtt.ewf.backend.dto.sync;

import com.fasterxml.jackson.annotation.JsonProperty;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;

/**
 * 设置命令下发请求体（Story 5.4 裁决 B）。
 *
 * <p><b>字段闭包（裁决 B）：</b>{@code volume}、{@code brightness}、{@code timeout}、
 * {@code action_id}、可选 {@code base_revision}。{@code @JsonProperty} 锁定 snake_case wire 名。
 * 取值域对齐契约 §9.1；{@code base_revision} 为 REST 乐观并发基线（契约未列）→ 已登记 deferred，
 * <b>不得</b>改 {@code docs/contracts/**}。
 *
 * <p><b>禁止：</b>客户端提交 {@code command_revision}/{@code applied_revision} 作为权威写入值——
 * 本 DTO 故意不包含这两字段。
 *
 * <p>幂等键为<strong>设置命令族</strong> {@code action_id}，落 {@code commands.json}；
 * 不得与 {@code progress.json} 篇章动作族 {@code action_id} 跨族复用。
 */
public record SettingsCommandRequest(
        @JsonProperty("volume")
        @NotNull(message = "volume 不能为空")
        @Min(value = 0, message = "volume 取值非法")
        @Max(value = 100, message = "volume 取值非法") Integer volume,
        @JsonProperty("brightness")
        @NotBlank(message = "brightness 不能为空")
        @Pattern(regexp = "low|mid|high", message = "brightness 取值非法") String brightness,
        @JsonProperty("timeout")
        @NotNull(message = "timeout 不能为空") Integer timeout,
        @JsonProperty("action_id") @NotBlank(message = "action_id 不能为空") String actionId,
        @JsonProperty("base_revision") Integer baseRevision) {
}
