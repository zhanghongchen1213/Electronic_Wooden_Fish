package top.zhcmqtt.ewf.backend.dto.sync;

import com.fasterxml.jackson.annotation.JsonProperty;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

/**
 * 篇章动作请求体（Story 5.2 裁决 D）。
 *
 * <p><b>字段闭包：</b>仅 {@code action}（{@code restart|exit}）与 {@code action_id}。
 * {@code action} 为 REST 实现裁决字段——契约未冻结该路径/字段名，已登记 deferred，
 * <b>不得</b>改 {@code docs/contracts/**}。
 *
 * <p>幂等键为篇章动作族 {@code action_id}，落 {@code progress.json} 单事务组；
 * 不得写入 {@code commands.json} 的设置命令族 {@code action_id}。
 */
public record RoundActionRequest(
        @JsonProperty("action")
        @NotBlank(message = "action 不能为空")
        @Pattern(regexp = "restart|exit", message = "action 取值非法") String action,
        @JsonProperty("action_id") @NotBlank(message = "action_id 不能为空") String actionId) {
}
