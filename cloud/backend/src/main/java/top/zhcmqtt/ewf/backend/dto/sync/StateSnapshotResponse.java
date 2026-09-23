package top.zhcmqtt.ewf.backend.dto.sync;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * 权威状态快照与恢复基准的查询响应体。
 *
 * <p><b>字段闭包（AC1）：</b>字段集合**恰为**契约 §6.2 的响应必填清单 ∪ `ws_frames[snapshot]`
 * 的 `payload_fields`，共 17 个 wire 字段，逐字取自
 * {@code docs/contracts/sync-contract.md}；此处用 {@link JsonProperty} 显式锁定 snake_case
 * wire 名，避免依赖 Jackson 对 record 组件的默认 camelCase 推导。字段名不得改名、加前缀或缩写，
 * 也不得新增 {@code delta}、{@code high_watermark}、{@code scripture_version}、{@code action_id}、
 * {@code replay_cursor}、{@code last_applied_seq}、{@code error} 等契约未授权的 wire 字段。
 *
 * <p><b>差量不是字段（AC1）：</b>按契约 §6.2，差量 = 响应 {@code acked_total} − 请求携带的基线
 * {@code acked_total}，是派生量；本类刻意不提供 {@code delta} 字段。
 *
 * <p><b>裁决 E（`round_id` 路由语义）：</b>本响应始终携带 `round_id` 与**轮次内游标**
 * `round_cursor`，从不返回跨轮的单一累计游标——客户端据此按 `round_id` 路由（契约 §8
 * 「事件携带 `round_id`，旧轮次事件不得写入当前篇章」），而不是按消息到达顺序。
 * 本 Story 不实现事件写入路径，故「旧轮次不写入当前轮次」在此的落点就是这个字段形状约束。
 *
 * <p><b>契约未冻结此面（本 Story 的内部一致性选择）：</b>空/未初始化状态的缺省表达不在契约内，
 * 因此把全部缺省值**集中**定义在本类的常量里（{@code NO_*} 与 {@code DEFAULT_*}），由
 * {@code StateSnapshotService} 单一装配点使用；不得在 DTO、服务与控制器三处各写一份。
 */
public record StateSnapshotResponse(
        @JsonProperty("device_id") String deviceId,
        @JsonProperty("acked_total") int ackedTotal,
        @JsonProperty("local_total") int localTotal,
        @JsonProperty("round_id") int roundId,
        @JsonProperty("round_state") String roundState,
        @JsonProperty("round_cursor") int roundCursor,
        @JsonProperty("pending_completion") boolean pendingCompletion,
        @JsonProperty("command_revision") int commandRevision,
        @JsonProperty("applied_revision") int appliedRevision,
        @JsonProperty("snapshot_seq") int snapshotSeq,
        @JsonProperty("battery_percent") int batteryPercent,
        @JsonProperty("network_mode") String networkMode,
        @JsonProperty("audio_config_version") int audioConfigVersion,
        @JsonProperty("firmware_version") String firmwareVersion,
        @JsonProperty("volume") int volume,
        @JsonProperty("brightness") String brightness,
        @JsonProperty("timeout") int timeout) {

    /** 计数与修订类的未初始化表达：契约把它们的取值域下界冻结为 0。 */
    public static final int NO_COUNT = 0;

    /** `pending_completion` 的未初始化表达：没有已确认轮次就不存在待完成置位。 */
    public static final boolean NO_PENDING_COMPLETION = false;

    /**
     * `round_id` 的未初始化表达。
     *
     * <p>契约把 `round_id` 的取值域冻结为 `integer ≥ 1`，因此**不能**回 0；而响应必填要求该字段
     * 必须在场。这里取域内最小值 1 作为「尚无已确认轮次」的占位，并且不同时伪造轮次事实：
     * `round_state` 取未完成态、`round_cursor` 取 0、`pending_completion` 取 false，四者合起来
     * 只表达「暂无已确认轮次」，不可能被误读为一个真实轮次。
     */
    public static final int NO_CONFIRMED_ROUND_ID = 1;

    /** `round_state` 的未初始化表达：契约取值域只有 `in_progress|completed`，取未完成态而非伪造完成。 */
    public static final String NO_ROUND_STATE = "in_progress";

    /** `battery_percent` 的未初始化表达：不伪造电量（尤其不伪造 100，那会掩盖未上报事实）。 */
    public static final int NO_BATTERY_PERCENT = 0;

    /**
     * `network_mode` 的未初始化表达。
     *
     * <p>契约取值域为 `connected|no_signal|disabled`，无 `unknown` 项。取 `no_signal` 而不是
     * `connected`：未收到设备上报时不得伪造在线事实，`no_signal` 与「无信号/未知」最接近且不宣称连通。
     */
    public static final String NO_NETWORK_MODE = "no_signal";

    /** `audio_config_version` 的未初始化表达。 */
    public static final int NO_AUDIO_CONFIG_VERSION = 0;

    /** `firmware_version` 的未初始化表达：空串，不伪造版本号。 */
    public static final String NO_FIRMWARE_VERSION = "";

    /** 待应用命令载荷默认值，逐值取自契约 §9.1（不是本 Story 的自造默认）。 */
    public static final int DEFAULT_VOLUME = 50;

    /** 待应用命令载荷默认值，逐值取自契约 §9.1（中档）。 */
    public static final String DEFAULT_BRIGHTNESS = "mid";

    /** 待应用命令载荷默认值，逐值取自契约 §9.1（15 秒）。 */
    public static final int DEFAULT_TIMEOUT = 15;
}
