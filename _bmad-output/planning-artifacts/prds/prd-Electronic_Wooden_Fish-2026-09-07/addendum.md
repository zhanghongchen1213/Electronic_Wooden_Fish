# 电子木鱼 PRD 附录

## 1. 文档职责

本附录保存 PRD 的决策反转、实现边界、验证门禁和文档索引。产品行为与 FR 只在同目录 `prd.md` 维护；硬件拓扑、GPIO、电源、总线和网络实现只在根目录 `architecture.md` 维护；分层输入/输出和跨层同步合同位于 `docs/`。`prd.md` 的功能需求按嵌入式层（Embedded）与软件层（Software，含后端 cloud/backend 与小程序 cloud/frontend）两层组织，FR 编号保留 E/B/F 前缀以保留子层归属。

## 2. 本轮决策覆盖

- 撤销“设备不播放声音/默认静音”的旧口径：ES8311＋NS4150B＋扬声器是核心反馈，默认音量 50。
- 顶层呈现由「Embedded / cloud/backend / cloud/frontend 三层平铺」调整为「嵌入式层 / 软件层」二层组织：嵌入式层含设备固件与硬件行为（FR-E-*），软件层含后端 cloud/backend（FR-B-*）与小程序 cloud/frontend（FR-F-*）；FR 编号与内容不重排、不更名，仅重组归属章节，便于下游分层阅读与追踪。
- 撤销 BLE→Wi‑Fi→4G 三模产品链路：MVP 只使用完整 Air780EGP 模组的 4G HTTPS JSON；ESP32-S3 的 BLE/Wi‑Fi 不作为业务通道；GPS 默认关闭。
- 撤销“屏幕只做状态显示”：设备采用木鱼、经文、统计三主页和下滑设置页。
- 新增 `device_touch` 正式输入；它与 `physical_pvdf` 共享同一个有效敲击队列。
- 木鱼页采用固定 7 个字形位置，最近已诵字左移，新字右入；标点占位但不消耗敲击，高速时动态加速且不丢正式字符。
- 小程序不提供电子木鱼点击，不产生正式敲击，只负责已确认经文的阅读行动画、统计、设备状态和命令。
- 设备熄屏时实体敲击直接计数并短亮屏；第一次触摸只唤醒；充电期间暂停输入和音频。
- QMI8658C 上板但默认不启用，只接 INT1→IO41；INT2 不接，GPIO45 不用于 IMU。
- Air780EGP 按木鱼专属 GPIO 合同重映射，保留屏幕和音频的 `legbot_watch` 资源：UART IO43/44、DTR IO10、RST IO15、NET_STATUS IO16、GNSS_VCC IO8、PVDF 比较器 IO11、RGB IO3。
- 后端持久化口径对齐架构主干 AD-16：由早期「单文件 SQLite」更新为「JSON 原子文件、零数据库」（临时文件 + fsync + rename，落 `cloud/backend/data/`），不引入任何数据库服务；PRD §2 分层图与 §6.1.9 FR-B-009 同步为 JSON 文件口径。

## 3. 技术细节边界

以下内容不在 PRD 中冻结具体料号，必须在原理图、供应商资料和样机中确认：

- PVDF 模拟前端、低功耗比较器、阈值、钳位和余振窗口；
- 单节锂电容量、充电保护、负载开关、3.3V/音频/AMOLED 稳压器；
- CO5300 目标 FPC 的供电、初始化、QSPI 时序、触摸盖板、亮屏功耗和 CST9217 地址；
- Air780EGP 发射峰值、活动窗口、PDP/HTTPS 复用与全天续航；
- 扬声器腔体、音量 50 的听感和高速连击节奏。

这些验证结果统一回写 `architecture.md` 和对应的 `docs/` 实现规格，不反向扩大产品 PRD 范围。

## 4. 下游文档索引

- `architecture.md`：硬件和跨层技术架构唯一事实源。
- `docs/README.md`：文档职责、权威顺序和变更流程。
- `docs/embedded/requirements.md`：固件输入、状态、屏幕、音频、电源和 4G 实现规格。
- `docs/backend/requirements.md`：幂等同步、游标、轮次、统计、命令和 JSON 持久化规格。
- `docs/frontend/requirements.md`：小程序动画、回放、统计、设备页和待设备应用规格。
- `docs/contracts/sync-contract.md`：跨层数据字段、事件来源、修订号、状态和错误合同。

分层文档不能复制完整 PRD，也不能自行定义新的事件来源、经文游标或同步状态。

## 5. 参考项目边界

### `legbot_watch`

可复用 CO5300/CST9217/CW2015/QMI8658C/ES8311/NS4150B BSP、共享 I²C、LVGL 多主页、状态栏、下滑设置、PWR 页面循环和亮屏时序。

不可照搬 ML307R、BLE 业务、外骨骼页面、四主页结构和 QMI8658C INT2→GPIO45 的板级合同。

### `main_control`

可复用 Air780EGP UART/AT、DTR 休眠、网络注册/PDP、HTTPS JSON、超时恢复、GPS 开关和状态模型。

不可照搬其外骨骼/景区业务 payload、原始 GPIO 常量和产品安全门禁。

## 6. 验证门禁

1. 校验 GPIO0/45/46 启动绑带、GPIO19/20 USB、GPIO9 ADC、GPIO11 比较器和 Air780EGP 重映射无冲突。
2. 验证单次、慢速连续和 1 秒 20 次 `physical_pvdf` / `device_touch` 输入最终计数、经文和音频一致。
3. 验证完成遮罩、充电暂停、熄屏首触、PWR 页面循环和 7 字滚动带。
4. 验证重复 HTTPS、离线恢复、backend 重启、小程序回放和 WebSocket 重连不造成重复或错序。
5. 验证设备无可信日期时不伪造今日统计；小程序命令在设备离线时显示待设备应用。

## 7. 历史与审计

旧版简报、旧 PRD 内容和本目录 `.memlog.md` 中的历史决策保留作为审计输入。新的反转和覆盖必须追加到 `.memlog.md`，不得通过删除旧行制造“从未做过旧决定”的假象。

