package top.zhcmqtt.ewf.backend.controller;

import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import jakarta.validation.Valid;
import jakarta.validation.constraints.Min;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.common.security.UserContext;
import top.zhcmqtt.ewf.backend.dto.sync.HistoryStatsResponse;
import top.zhcmqtt.ewf.backend.dto.sync.RoundActionRequest;
import top.zhcmqtt.ewf.backend.dto.sync.SettingsCommandRequest;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.dto.sync.SyncReportRequest;
import top.zhcmqtt.ewf.backend.service.HistoryStatsService;
import top.zhcmqtt.ewf.backend.service.ProgressSyncService;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;

/**
 * 权威状态查询、高水位幂等上报、篇章动作、历史统计与设置命令下发入口。
 *
 * <p><b>裁决 B（查询路径，Story 4.6）：</b>{@code GET /api/v1/sync/snapshot} — 契约未冻结 REST
 * 查询路径，本路径由 4.6 裁决；语义保持只读，本 Story 不改。
 *
 * <p><b>裁决 D（上报路径，Story 5.1）：</b>{@code POST /api/v1/sync/report} — 请求体 = 契约 §6.1
 * 必填闭包。**该路径未被契约冻结**，已登记 deferred，不得改契约。
 *
 * <p><b>裁决 D（篇章动作路径，Story 5.2）：</b>{@code POST /api/v1/sync/round-action} — body =
 * {@code {action, action_id}}。理由：与 {@code /report} 同控制器族；避免把 restart/exit 塞进 §6.1
 * 上报闭包。{@code action} 为 REST 裁决字段（契约未冻结）→ 已登记 deferred，不得改契约。
 *
 * <h2>裁决 D（Story 5.3）：历史统计路径</h2>
 * <p><b>选择：</b>{@code GET /api/v1/sync/stats}，无必填 query；身份取
 * {@link UserContext#currentDeviceId()}（MVP 单设备，统计落在共享 data-dir）。
 * <b>理由：</b>与现有 sync 控制器族一致；记录页（6.6）只读消费；不污染 §6.1 上报闭包与 17 字段快照。
 * <b>约束：</b>只读——不得在查询路径创建目录/写文件/catch-up 写盘；路径未入契约 → 登记 deferred。
 *
 * <h2>裁决 A（Story 5.4）：设置命令路径</h2>
 * <p><b>选择：</b>{@code POST /api/v1/sync/command} — body = {@link SettingsCommandRequest}
 *（{@code volume}/{@code brightness}/{@code timeout}/{@code action_id}/可选 {@code base_revision}）。
 * <b>理由：</b>与 {@code /report}、{@code /round-action}、{@code /stats} 同控制器族；语义是「下发设置命令」
 * 而非篇章动作；契约未冻结 REST 路径与 {@code base_revision} → 登记 deferred，不得改契约。
 * <b>约束：</b>身份只取 {@link UserContext#currentDeviceId()}；修订算法在
 * {@link ProgressSyncService#submitSettings}，控制器不散落。
 *
 * <p><b>身份：</b>单设备内省——{@code device_id} 只取自 {@link UserContext#currentDeviceId()}；
 * 请求体中的身份声明一律不可信。全部路径默认受 {@code JwtAuthenticationFilter} 保护，**不新增白名单**。
 *
 * <p><b>拒绝仍带基准（裁决 C）：</b>业务拒绝走 {@link ApiResponse#error(int, String, Object)}；
 * 成功走 {@link ApiResponse#success(Object)}。
 *
 * <p><b>本 Story（5.4）交付：</b>{@code POST /command} 设置命令修订递增与 {@code 20006} CAS。
 * <p><b>Story 5.5：</b>WebSocket 差量推送由独立 handler（{@code /api/v1/ws}）承载，
 * <b>不</b>把 WS 塞进本 MVC 控制器；确认后推送钩子在 {@link ProgressSyncService}。
 * <b>仍不做：</b>「立即同步」、小程序 UI 消费。
 */
@Validated
@RestController
@RequestMapping("/api/v1/sync")
public class SyncController {

    private final StateSnapshotService stateSnapshotService;
    private final ProgressSyncService progressSyncService;
    private final HistoryStatsService historyStatsService;

    public SyncController(StateSnapshotService stateSnapshotService, ProgressSyncService progressSyncService,
            HistoryStatsService historyStatsService) {
        this.stateSnapshotService = stateSnapshotService;
        this.progressSyncService = progressSyncService;
        this.historyStatsService = historyStatsService;
    }

    /**
     * 返回带快照水位的权威状态与差量操作数。
     *
     * <p>参数校验用 Spring MVC 标准能力（{@code @RequestParam(required = false)} + 方法参数约束），
     * 非法取值仍由既有映射回 {@code 400}/{@code 40000}，不在控制器内手写校验错误响应、不自造码。
     */
    @GetMapping("/snapshot")
    public ApiResponse<StateSnapshotResponse> snapshot(
            @RequestParam(value = "acked_total", required = false) @Min(value = 0, message = "acked_total 不可为负") Integer ackedTotal,
            @RequestParam(value = "snapshot_seq", required = false) @Min(value = 0, message = "snapshot_seq 不可为负") Integer snapshotSeq) {
        StateSnapshotService.SnapshotOutcome outcome = stateSnapshotService.resolve(
                UserContext.currentDeviceId(), ackedTotal, snapshotSeq);
        if (outcome.accepted()) {
            return ApiResponse.success(outcome.snapshot());
        }
        return ApiResponse.error(outcome.code(), outcome.message(), outcome.snapshot());
    }

    /**
     * 历史统计查询（裁决 D / Story 5.3）。只读；正式数字不含未确认差。
     */
    @GetMapping("/stats")
    public ApiResponse<HistoryStatsResponse> stats() {
        // 触发 JWT 身份解析；MVP 单设备，统计不按 deviceId 分文件。
        UserContext.currentDeviceId();
        return ApiResponse.success(historyStatsService.query());
    }

    /**
     * 高水位幂等上报（裁决 D / Story 5.1）。
     *
     * <p>body 校验走 {@code @Valid @RequestBody} → {@code MethodArgumentNotValidException} →
     * 既有 {@code 40000} 映射；不与类级 {@code @Validated} 的方法参数约束混成第二套错误体。
     */
    @PostMapping("/report")
    public ApiResponse<StateSnapshotResponse> report(@Valid @RequestBody SyncReportRequest request) {
        ProgressSyncService.SyncOutcome outcome = progressSyncService.report(
                UserContext.currentDeviceId(), request);
        if (outcome.accepted()) {
            return ApiResponse.success(outcome.snapshot());
        }
        return ApiResponse.error(outcome.code(), outcome.message(), outcome.snapshot());
    }

    /**
     * 篇章动作：从头开始 / 退出（裁决 D / Story 5.2）。
     *
     * <p>拒绝走 {@link ApiResponse#error(int, String, Object)} 携带完整 17 字段快照；
     * 不新增 JWT 白名单。
     */
    @PostMapping("/round-action")
    public ApiResponse<StateSnapshotResponse> roundAction(@Valid @RequestBody RoundActionRequest request) {
        ProgressSyncService.SyncOutcome outcome = progressSyncService.roundAction(
                UserContext.currentDeviceId(), request);
        if (outcome.accepted()) {
            return ApiResponse.success(outcome.snapshot());
        }
        return ApiResponse.error(outcome.code(), outcome.message(), outcome.snapshot());
    }

    /**
     * 设置命令下发（裁决 A / Story 5.4）。
     *
     * <p>拒绝（含 {@code 20006}）走 {@link ApiResponse#error(int, String, Object)} 携带完整 17 字段快照；
     * 不新增 JWT 白名单；修订算法在 {@link ProgressSyncService#submitSettings}。
     */
    @PostMapping("/command")
    public ApiResponse<StateSnapshotResponse> command(@Valid @RequestBody SettingsCommandRequest request) {
        ProgressSyncService.SyncOutcome outcome = progressSyncService.submitSettings(
                UserContext.currentDeviceId(), request);
        if (outcome.accepted()) {
            return ApiResponse.success(outcome.snapshot());
        }
        return ApiResponse.error(outcome.code(), outcome.message(), outcome.snapshot());
    }
}
