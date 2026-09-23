package top.zhcmqtt.ewf.backend.ws;

import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;

/**
 * 确认后实时推送窄口（Story 5.5）。
 *
 * <p><b>裁决 E（推送时机与失败策略）：</b>由 {@code ProgressSyncService} 在写锁释放且权威状态已可读之后调用；
 * 推送异常只打中文日志，不影响 HTTP 成功确认；无订阅者时为 no-op（不抛、不写盘）。
 * 禁止在 Store 层推送；禁止在未确认的失败/拒绝路径（如 {@code 20003}）调用本口。
 */
public interface RealtimePushPort {

    /** 无订阅者 / 单测替身：全部 no-op。 */
    RealtimePushPort NOOP = (deviceId, snapshot, delta, completion, commandState) -> {
    };

    /**
     * 按标志推送零或多帧。载荷一律取自已确认的 {@code snapshot}（与同刻 REST 快照同源）。
     *
     * @param deviceId      已鉴权设备身份
     * @param snapshot      写成功后的权威快照
     * @param delta         是否推 {@code delta}（acked 相对写前前进）
     * @param completion    是否推 {@code completion}（本轮刚确认完成）
     * @param commandState  是否推 {@code command_state}（命令修订或已生效翻转）
     */
    void dispatch(String deviceId, StateSnapshotResponse snapshot, boolean delta, boolean completion,
            boolean commandState);
}
