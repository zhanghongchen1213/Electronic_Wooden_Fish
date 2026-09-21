# Reviewer Gate：版本与现实核验（复审）

审查对象：`ARCHITECTURE-SPINE.md`（2026-09-22 当前工作树）及 AD-20 所引用的配置、脚本与交付契约。此次复审区分“绿地阶段尚未创建实现工程”和“契约自身存在现实阻断”。未修改主干文件。

## Verdict

**NEEDS FIX（条件不通过）**。本轮端口、生产域名占位、脚本根目录优先级、JDK/Maven 版本门禁、local profile 固定和 SHA-256 sidecar 方向已与架构对齐；`cloud/backend` 尚无 `pom.xml`/`target` 属绿地工程的预期限制，不单独作为 Reviewer Gate 阻断。但上传认证仍把 SSH 密码写入仓库且关闭 host-key 校验，真实构建/部署前必须修正或取得明确的临时豁免。

## 已核对且当前一致

- 架构 AD-18/AD-20、`application.yml`、`application-local.yml`、`.env.development`、`start-local-test-backend.sh`、`stop.sh` 均使用本地端口 `9218`。
- `build-prod-backend.sh` 已检查 Java 主版本必须为 17、Maven 必须为 3.9.x；`start-local-test-backend.sh` 已将 `SPRING_PROFILE` 固定为 `local`，并在构建路径检查相同的 JDK/Maven 门禁。
- `.env` 已恢复为 `https://replace-with-ewf-api-domain.example/api/v1` 占位值，当前没有可验证的 EWF 生产域名或证书承诺；TLS 现实核验应留到真实域名填入后再做。
- `build-prod-backend.sh` 与 `upload-backend-jars.sh` 都要求 `saas.jar` 旁的 `saas.jar.sha256`；上传前重新计算 SHA-256 并上传两者，基本闭合了同名 Jar 误传风险。
- 构建脚本的显式 CI 根目录优先级、脚本所在 checkout 回溯和上传脚本的脚本目录优先级已指向 EWF，而不是当前工作目录中的另一份 checkout。
- 四个脚本均通过 `bash -n`；但这只说明语法正确，不等于构建或起停已验证。

## 绿地限制（不单独判阻断）

当前 `cloud/backend/` 只有 `src/main/resources/application*.yml`，没有 `pom.xml`、Java 源码或 `target/saas.jar`；`cloud/frontend/` 只有 `.env` 文件，没有 `package.json` 或 uni-app 工程。在仓库根执行构建脚本仍退出码 1，并提示缺少 `cloud/backend/pom.xml`。这是当前 planning/绿地状态的事实，不应伪装成“构建已通过”；但只要架构明确 AD-20 是后续实现契约，而非已落地产物，本身不是本轮文档 Review 的 P0。

## Findings

### 当前通过项 — JDK/Maven 与 local profile 门禁已补齐

`build-prod-backend.sh` 现在显式拒绝非 Java 17 和非 Maven 3.9.x；`start-local-test-backend.sh` 将 `SPRING_PROFILE="local"` 固定，并在执行构建时复用同一 Maven 3.9.x 检查。脚本注释也已从精确 Maven 3.9.5 改为 3.9.x。当前仍无法跑到这些门禁后的 Maven 构建，因为绿地仓库没有 `cloud/backend/pom.xml`；这属于预期限制，待工程落地后再做成功回执。

### P1 — 上传认证仍违反仓库安全边界

`upload-backend-jars.sh:74-77` 仍包含硬编码 SSH 密码；`SSH_OPTS`（`90-94`）还关闭了 `StrictHostKeyChecking` 并把 known-hosts 指向 `/dev/null`。这与 `cloud/AGENTS.md` 要求密钥从受控配置读取的规则冲突，也使 SHA-256 sidecar 只证明本地文件完整，不能证明上传目标可信。架构虽把明文密钥列为原型风险接受项，但这只能是未部署前的临时状态。

修正：使用 CI secret/SSH key 和受控 known_hosts（或指纹 pinning），轮换现有凭据；真实 Huawei Cloud/公网部署前，该项是阻断条件。

### P2 — Spring Boot 3.3.7 仍是 EOL 来源基线，尚未成为 EWF 锁定事实

`ARCHITECTURE-SPINE.md:160,204,213,295` 仍写 Java 17 + Spring Boot 3.3.7，并承认 3.3.x 已于 2025-06-30 EOL；当前没有 EWF `pom.xml`，所以无法证明该版本已锁定。Deferred 已给出升级方向，绿地 planning 阶段可接受，但进入实现/部署前必须重新选择并记录支持线。

### P2 — 产物名称与 sidecar 已声明，但仍需实现后验证

AD-20（`ARCHITECTURE-SPINE.md:184,193,304`）要求 `saas.jar` + `saas.jar.sha256`，脚本按该名称生成、校验和上传。由于当前没有 `pom.xml`，尚无法证明 Maven 最终产物确实重命名为 `saas.jar`，也无法验证 sidecar 在 Linux CI 和 macOS 本地的格式兼容。创建 EWF `pom.xml` 后应显式固定 finalName，并核对两端摘要及远端回执。

## 通过条件

1. EWF backend/frontend 工程落地后，构建脚本成功生成并校验 `target/saas.jar` 与 `target/saas.jar.sha256`；绿地缺失工程不再被误报为已验证。
2. 在 EWF backend 工程落地后，保留并实测 JDK 17/Maven 3.9.x 门禁及 `local` profile 起停。
3. 上传认证改为受控 secret/key，恢复 host-key 校验或指纹 pinning，并轮换现有凭据。
4. Spring Boot 版本在 EWF `pom.xml` 中实际锁定；若继续使用 EOL 3.3.7，保留明确升级门禁。
5. 真实生产域名填入 `.env` 后，再用不跳过证书校验的 HTTPS/WSS 请求完成域名与证书回执。
