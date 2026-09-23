package top.zhcmqtt.ewf.backend.controller;

import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import jakarta.validation.constraints.Min;
import top.zhcmqtt.ewf.backend.common.response.ApiResponse;
import top.zhcmqtt.ewf.backend.common.security.UserContext;
import top.zhcmqtt.ewf.backend.dto.sync.StateSnapshotResponse;
import top.zhcmqtt.ewf.backend.service.StateSnapshotService;

/**
 * 权威状态查询入口：`GET /api/v1/sync/snapshot`（裁决 B）。
 *
 * <p><b>裁决 B（路径与请求面）：</b>契约没有冻结任何 REST 查询路径——§12 只冻结 `/api/v1` 前缀与
 * `{code,message,data}` 信封，§10.2 只派生 WebSocket 路径，注册表 `endpoint` 也只描述 WebSocket。
 * 因此路径由本 Story 裁决：`/api/v1/sync/snapshot`。三个词都直接取自契约自身词汇——
 * `/api/v1` 是 §12 冻结前缀，`snapshot` 是 §10.1 的帧类型（其出处即 FR-C-006），GET 表达纯读取，
 * 无需发明请求体。**该路径未被契约冻结**，已作为契约缺口登记，交 Epic 5 对齐（Epic 5 的 §6.2
 * 上报响应必须复用同一个 {@link StateSnapshotResponse}，不得再造第二套形状）。
 *
 * <p><b>请求面（裁决 B）：</b>只接受契约已冻结的**基线操作数**作为可选参数——
 * {@code acked_total}（§6.2 的差量操作数）与 {@code snapshot_seq}（§8 的续订基准）。
 * 两者都只是「客户端声明的基准」，一律按**未经核实的声明**处理：只用于比较，绝不写入、绝不改变
 * 任何权威字段。可选参数不得改名为 `displayed_total`、`client_total`、`cursor` 等自造名，
 * 请求面也不得新增契约未授权的字段。
 *
 * <p><b>身份（AC3 / Task 3.3）：</b>单设备内省——`device_id` 只取自
 * {@link UserContext#currentDeviceId()}（由 `JwtAuthenticationFilter` 依落盘身份校验后写入），
 * 请求体或参数中的任何身份声明一律不可信。新路径默认受该过滤器保护，**不新增白名单**；
 * 也不改动过滤器或 `SecurityConfig` 的既有语义。
 *
 * <p><b>拒绝仍带基准（裁决 C）：</b>业务拒绝（`20001`/`20002`/`20003`）时 `data` 必须携带完整权威
 * 快照（契约 §12），因此走 {@link ApiResponse#error(int, String, Object)} 而不是抛
 * `BusinessException`（后者经异常层只产出 {@code {code,message}}）。业务错误保持 HTTP 200 + 业务码，
 * 与 §12 的 `http_status` 一致。
 *
 * <p><b>本 Story 不做：</b>不实现 WebSocket 续订、`seq` 帧空间与断线补齐流程（Epic 5）；
 * 不实现任何写路径（不推进高水位、不改轮次/游标/命令修订）。
 */
@Validated
@RestController
@RequestMapping("/api/v1/sync")
public class SyncController {

    private final StateSnapshotService stateSnapshotService;

    public SyncController(StateSnapshotService stateSnapshotService) {
        this.stateSnapshotService = stateSnapshotService;
    }

    /**
     * 返回带快照水位的权威状态与差量操作数。
     *
     * <p>参数校验用 Spring MVC 标准能力（{@code @RequestParam(required = false)} + 方法参数约束），
     * 非法取值仍由既有映射回 `400`/`40000`，不在控制器内手写校验错误响应、不自造码。
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
}
