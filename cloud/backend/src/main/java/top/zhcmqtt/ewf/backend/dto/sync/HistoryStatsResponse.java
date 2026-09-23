package top.zhcmqtt.ewf.backend.dto.sync;

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * 历史统计查询响应（Story 5.3 裁决 C）。
 *
 * <p><b>选择（wire snake_case）：</b>{@code today_taps}、{@code last_7_days_taps}、
 * {@code last_30_days_taps}、{@code total_taps}、{@code streak_days}、{@code empty}、
 * {@code pending_sync}。
 * <b>理由：</b>正式数字只来自已确认桶与权威 {@code acked_total}；空态/待同步显式表达，避免「全 0」
 * 伪装成有记录。
 * <b>约束：</b>本 DTO <b>不在</b>契约注册表——已登记 deferred；不得塞进 17 字段
 * {@link StateSnapshotResponse}；不得新增 {@code ErrorCode}。
 *
 * <p>窗口：{@code last_7}/{@code last_30} 含今日共 7/30 个上海自然日（今日往前）。
 * {@code total_taps} := 权威 {@code acked_total}（查询以 progress 为准，避免派生滞后时数字分裂）。
 */
public record HistoryStatsResponse(
        @JsonProperty("today_taps") int todayTaps,
        @JsonProperty("last_7_days_taps") int last7DaysTaps,
        @JsonProperty("last_30_days_taps") int last30DaysTaps,
        @JsonProperty("total_taps") int totalTaps,
        @JsonProperty("streak_days") int streakDays,
        @JsonProperty("empty") boolean empty,
        @JsonProperty("pending_sync") boolean pendingSync) {
}
