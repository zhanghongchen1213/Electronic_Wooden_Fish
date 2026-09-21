/**
 * @file     audio_service.h
 * @brief    类型化音频播放、UI 反馈与语音采集服务接口
 * @details  定义固定音频资源、点击音、触觉反馈、语音 PCM 流、稳定结果分类及 SPIFFS 资源生命周期入口。
 * @author   ZHC
 * @date     2026-08-05
 */

#ifndef LEGBOT_AUDIO_SERVICE_H
#define LEGBOT_AUDIO_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"
#include "legbot_services.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 音频服务在统一服务表中的固定 ID。 */
#define LEGBOT_AUDIO_SERVICE_ID LEGBOT_SERVICE_AUDIO
/** SPIFFS 在 VFS 中的只读挂载路径。 */
#define AUDIO_SERVICE_SPIFFS_BASE_PATH "/spiffs"
/** 音频资源所在 SPIFFS 分区标签。 */
#define AUDIO_SERVICE_SPIFFS_PARTITION_LABEL "spiffs"
/** 音频服务允许同时打开的最大文件数。 */
#define AUDIO_SERVICE_SPIFFS_MAX_FILES 2
/** typed 音频命令队列深度。 */
#define AUDIO_SERVICE_QUEUE_DEPTH 4
/** UI 按钮反馈队列深度。 */
#define AUDIO_SERVICE_UI_FEEDBACK_QUEUE_DEPTH 8
/** 单次读取与写入的 PCM 分块大小，对应 16 kHz/16-bit/mono 的 8 ms 数据。 */
#define AUDIO_SERVICE_PCM_CHUNK_BYTES 256U
/** 每次 I2S 写入的有限等待上限。 */
#define AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS 10U
/** storage/assets 从进入阶段到得出挂载/资源判定的验收窗口。 */
#define AUDIO_SERVICE_STORAGE_ACCEPTANCE_MS 3000U
/** play 请求接受到 terminal state 的验收窗口。 */
#define AUDIO_SERVICE_PLAYBACK_ACCEPTANCE_MS 10000U
/** 连续处理区段的目标上限，单位微秒。 */
#define AUDIO_SERVICE_MAX_PROCESSING_US 20000U
/** PA 安全关断和关键 terminal 状态发布的有界重试次数。 */
#define AUDIO_SERVICE_CRITICAL_RETRY_COUNT 3U
/** 语音 PCM 固定帧大小，对应 16 kHz/16-bit/mono 的 32 ms 数据。 */
#define AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES 1024U
/** 语音 PCM Stream Buffer 可容纳的完整帧数。 */
#define AUDIO_SERVICE_VOICE_PCM_FRAME_COUNT 8U
/** 提示音结束且 PA 关闭后的 ADC 启动稳定时间。 */
#define AUDIO_SERVICE_VOICE_CAPTURE_SETTLE_MS 20U
/** 单次语音采集的最长持续时间。 */
#define AUDIO_SERVICE_VOICE_CAPTURE_MAX_MS 5000U
/** 单次 I2S RX 读取的有限等待上限。 */
#define AUDIO_SERVICE_I2S_READ_TIMEOUT_MS 40U
/** 语音会话事件队列深度。 */
#define AUDIO_SERVICE_VOICE_EVENT_QUEUE_DEPTH 8U

    typedef enum
    {
        AUDIO_RESOURCE_SELFTEST_OK = 0, /**< 固定的 16 kHz 单声道自检提示音。 */
        AUDIO_RESOURCE_UI_CLICK = 1,    /**< 固定的 16 kHz 单声道柔和点击音。 */
        AUDIO_RESOURCE_VOICE_START = 2, /**< 固定的 16 kHz 单声道语音进入提示音。 */
        AUDIO_RESOURCE_COUNT,           /**< 音频资源数量。 */
    } audio_resource_id_t;

    typedef enum
    {
        AUDIO_RESULT_OK = 0,           /**< 操作成功。 */
        AUDIO_RESULT_NOT_READY,        /**< SPIFFS 或音频资源尚未准备。 */
        AUDIO_RESULT_RESOURCE_MISSING, /**< typed 资源对应文件不存在。 */
        AUDIO_RESULT_IO_FAILED,        /**< 文件系统或文件 I/O 失败。 */
        AUDIO_RESULT_CORRUPT,          /**< RIFF/WAV 结构截断、越界或损坏。 */
        AUDIO_RESULT_UNSUPPORTED,      /**< WAV 编码参数不属于本 Story 支持范围。 */
        AUDIO_RESULT_BUSY,             /**< 已有播放或请求队列没有空间。 */
        AUDIO_RESULT_HW_FAILED,        /**< codec、I2S 或功放操作失败。 */
        AUDIO_RESULT_CODEC_FAILED,     /**< ES8311 配置或清理失败。 */
        AUDIO_RESULT_I2S_FAILED,       /**< I2S0 启停或写入失败。 */
        AUDIO_RESULT_PA_FAILED,        /**< NS4150 安全控制失败。 */
        AUDIO_RESULT_PCM_OVERFLOW,     /**< 语音 PCM 缓存不足且本次会话已失败关闭。 */
    } audio_service_result_t;

    typedef enum
    {
        AUDIO_PRIORITY_DEFAULT = 0, /**< 默认播放优先级。 */
        AUDIO_PRIORITY_SELFTEST,    /**< 固定自检播放优先级。 */
    } audio_service_priority_t;

    typedef enum
    {
        AUDIO_SERVICE_REQUEST_PLAY = 0,        /**< 播放 typed 资源。 */
        AUDIO_SERVICE_REQUEST_STOP = 1,        /**< 中断播放并退出 audio_task。 */
        AUDIO_SERVICE_REQUEST_CANCEL_PLAYBACK, /**< 仅取消当前或待执行播放。 */
        AUDIO_SERVICE_REQUEST_VOICE_CAPTURE_START, /**< 播放固定提示音后开始语音采集。 */
    } audio_service_request_type_t;

    typedef struct
    {
        audio_service_request_type_t type; /**< typed 音频命令类型。 */
        union
        {
            struct
            {
                audio_resource_id_t resource_id;   /**< 待播放的固定资源 ID。 */
                audio_service_priority_t priority; /**< 本 Story 允许的播放优先级。 */
                int64_t accepted_at_us;            /**< play 请求成功接受的单调时钟。 */
                uint32_t generation;               /**< 非零播放代次，用于隔离过期取消。 */
            } play;                                /**< 播放命令载荷。 */
            struct
            {
                int64_t requested_at_us; /**< stop 请求入队时刻。 */
            } stop;                      /**< 停止命令载荷。 */
            struct
            {
                int64_t requested_at_us; /**< cancel 请求入队时刻。 */
                uint32_t target_generation; /**< 仅允许取消的目标播放代次。 */
            } cancel;                    /**< 取消当前播放命令载荷。 */
            struct
            {
                uint32_t generation; /**< 非零语音会话代次。 */
            } voice;                /**< 语音采集命令载荷。 */
        } payload;                       /**< 按 type 解释的命令载荷。 */
    } audio_service_request_t;

    typedef enum
    {
        AUDIO_VOICE_EVENT_CUE_STARTED = 0, /**< 已开始播放固定进入提示音。 */
        AUDIO_VOICE_EVENT_CAPTURE_STARTED, /**< ADC/RX 已启动且可读取 PCM。 */
        AUDIO_VOICE_EVENT_CAPTURE_STOPPED, /**< 已按释放请求正常停止采集。 */
        AUDIO_VOICE_EVENT_CANCELLED,       /**< 在采集开始前已释放并取消。 */
        AUDIO_VOICE_EVENT_TIMED_OUT,       /**< 已达到五秒采集上限。 */
        AUDIO_VOICE_EVENT_FAILED,          /**< 提示音、ADC、I2S 或缓存失败。 */
    } audio_service_voice_event_type_t;

    typedef struct
    {
        audio_service_voice_event_type_t type; /**< 语音会话状态事件。 */
        uint32_t generation;                   /**< 事件所属的非零会话代次。 */
        audio_service_result_t result;         /**< 状态对应的稳定音频结果。 */
    } audio_service_voice_event_t;

    typedef struct
    {
        uint32_t storage_verdict_ms;        /**< storage/assets 进入到资源判定的墙钟时间。 */
        uint32_t playback_elapsed_ms;       /**< 最近 play 接受到 terminal state 的墙钟时间。 */
        uint32_t max_processing_us;         /**< 分块循环最大连续非阻塞处理时间。 */
        uint32_t stop_latency_ms;           /**< 最近 stop 从入队到 audio_task 观测的延迟。 */
        UBaseType_t stack_high_water_words; /**< audio_task 最小剩余栈高水位，单位 word。 */
        uint32_t voice_rx_timeout_count;    /**< 最近一次语音会话的 I2S RX timeout 次数。 */
    } audio_service_metrics_t;

    typedef struct
    {
        uint32_t sample_rate_hz;  /**< WAV 采样率。 */
        uint32_t byte_rate;       /**< WAV 每秒 PCM 字节数。 */
        uint32_t data_offset;     /**< PCM data chunk 在文件内的偏移。 */
        uint32_t data_size;       /**< PCM data chunk 字节数。 */
        uint16_t audio_format;    /**< WAV 编码格式，当前仅允许 PCM=1。 */
        uint16_t channels;        /**< WAV 声道数量，当前仅允许单声道。 */
        uint16_t block_align;     /**< 每个采样帧字节数。 */
        uint16_t bits_per_sample; /**< 每个采样位数，当前仅允许 16 位。 */
    } audio_wav_info_t;

    typedef struct
    {
        bool play_haptics;    /**< 本次最终是否执行触觉反馈。 */
        bool play_click_audio; /**< 本次最终是否播放点击音。 */
    } audio_ui_feedback_output_t;

    /**
     * @brief 判定指定电量等级下是否允许固定音频输出
     * @details 严重低电只保留 AUDIO_PRIORITY_SELFTEST，非法等级或优先级一律拒绝。
     * @param power_level power owner 已采用的闭集电量等级
     * @param priority 播放请求优先级
     * @return true 允许输出，false 静默抑制
     */
    bool audio_service_output_allowed(
        watch_power_level_t power_level,
        audio_service_priority_t priority);

    /**
     * @brief 判定指定电量等级下是否允许非必要 UI 音振反馈
     * @param power_level power owner 已采用的闭集电量等级
     * @return true 允许点击音和触觉反馈，false 静默抑制
     */
    bool audio_service_ui_feedback_allowed(
        watch_power_level_t power_level);

    /**
     * @brief 合并点击时请求快照与消费时最新音振设置
     * @details 任一通道只在点击时请求且消费时仍开启时执行，关闭设置可抑制排队旧请求。
     * @param requested_haptics 点击发生时是否请求触觉反馈
     * @param requested_click_audio 点击发生时是否请求点击音
     * @param current_haptics 消费请求时触觉反馈是否仍开启
     * @param current_click_audio 消费请求时点击音是否仍开启
     * @return 两个通道最终应执行的独立闭集结果
     */
    audio_ui_feedback_output_t audio_service_resolve_ui_feedback(
        bool requested_haptics,
        bool requested_click_audio,
        bool current_haptics,
        bool current_click_audio);

    /**
     * @brief 挂载打包资源并严格探测固定 WAV
     * @param result typed 资源探测结果输出
     * @return ESP_OK 资源已准备
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         其他错误表示音频资源降级，调用者可继续核心启动
     */
    esp_err_t audio_service_storage_init(audio_service_result_t *result);

    /**
     * @brief 幂等注销由音频服务拥有的 SPIFFS 挂载
     * @return ESP_OK 已注销或本模块没有挂载所有权
     *         其他错误表示底层注销失败，所有权保留以便重试
     */
    esp_err_t audio_service_storage_deinit(void);

    /**
     * @brief 查询固定资源是否通过挂载与 WAV 格式探测
     * @return true 资源可播放，false 资源不可用
     */
    bool audio_service_storage_ready(void);

    /**
     * @brief 将 typed 资源 ID 映射为编译期固定绝对路径
     * @param resource_id 音频资源 ID
     * @return 固定路径；资源 ID 非法时返回 NULL
     */
    const char *audio_service_resource_path(audio_resource_id_t resource_id);

    /**
     * @brief 打开并严格解析一个 typed WAV 资源
     * @param resource_id 音频资源 ID
     * @param info 成功时输出 WAV 元数据
     * @return typed 资源、I/O、损坏或不支持结果
     */
    audio_service_result_t audio_service_probe_resource(audio_resource_id_t resource_id,
                                                        audio_wav_info_t *info);

    /**
     * @brief 获取音频结果的稳定 ASCII 错误码
     * @param result 音频结果
     * @return AUDIO_ 前缀稳定错误码
     */
    const char *audio_service_result_code(audio_service_result_t result);

    /**
     * @brief 创建 audio_service 私有 typed 队列
     * @return ESP_OK 成功或已创建，ESP_ERR_NO_MEM 创建失败
     */
    esp_err_t audio_service_init_contracts(void);

    /**
     * @brief 在框架创建 audio_task 前进入 starting 状态并清理旧命令
     * @return ESP_OK 已准备，ESP_ERR_INVALID_STATE 队列不可用或任务仍在运行
     */
    esp_err_t audio_service_prepare_run(void);

    /**
     * @brief 回滚未成功创建的 audio_task starting 状态
     * @details 仅供统一服务框架在 xTaskCreate 失败路径调用。
     */
    void audio_service_cancel_prepared_run(void);

    /**
     * @brief 删除未运行 audio_task 的 typed 队列
     * @return ESP_OK 成功或尚未创建，ESP_ERR_INVALID_STATE 正在播放
     */
    esp_err_t audio_service_deinit_contracts(void);

    /**
     * @brief 投递固定资源播放请求
     * @param resource_id typed 资源 ID
     * @param priority 默认或自检优先级
     * @param timeout_ticks 等待队列空间的有限 tick 数
     * @return AUDIO_RESULT_OK 已入队，AUDIO_RESULT_BUSY 已有播放/排队或队列满
     */
    audio_service_result_t audio_service_request_play(audio_resource_id_t resource_id,
                                                      audio_service_priority_t priority,
                                                      TickType_t timeout_ticks);

    /**
     * @brief 非阻塞投递一次 UI 按钮触觉和点击音反馈
     * @details 保存点击时的设置快照，并以最新策略抑制关闭后的旧请求；两项均关闭时不入队。
     * @param haptics_enabled 本次点击是否请求触觉反馈
     * @param click_audio_enabled 本次点击是否请求点击音
     * @return AUDIO_RESULT_OK 已入队或按关闭设置静默
     *         AUDIO_RESULT_BUSY UI 反馈队列已满
     *         AUDIO_RESULT_NOT_READY audio_task 尚未运行或正在停止
     */
    audio_service_result_t audio_service_request_ui_feedback(
        bool haptics_enabled,
        bool click_audio_enabled);

    /**
     * @brief 取消当前或已排队的固定资源播放但保持 audio_task 运行
     * @param timeout_ticks 等待队列空间的有限 tick 数
     * @return ESP_OK 已确认取消或当前没有播放
     *         ESP_ERR_INVALID_STATE 队列未创建、任务未运行或正在停止
     *         ESP_ERR_TIMEOUT 队列满或在期限内未收到 audio_task 确认
     */
    esp_err_t audio_service_request_cancel_playback(TickType_t timeout_ticks);

    /**
     * @brief 请求 audio owner 播放固定提示音并开始语音采集
     * @param generation 调用方生成的非零会话代次
     * @return ESP_OK 已入队
     *         ESP_ERR_INVALID_ARG 代次为零
     *         ESP_ERR_INVALID_STATE 服务、资源或硬件未就绪
     *         ESP_ERR_TIMEOUT 音频忙或队列已满
     */
    esp_err_t audio_service_request_voice_capture_start(uint32_t generation);

    /**
     * @brief 请求停止同一代次的待启动或活动语音采集
     * @details 通过原子代次立即取消提示音或 RX，过期代次不会影响新会话。
     * @param generation 目标非零会话代次
     * @return ESP_OK 已记录停止或该代次已经结束
     *         ESP_ERR_INVALID_ARG 代次为零
     *         ESP_ERR_INVALID_STATE 服务未运行或代次不匹配
     */
    esp_err_t audio_service_request_voice_capture_stop(uint32_t generation);

    /**
     * @brief 接收 audio owner 发布的语音会话事件
     * @param event 事件输出
     * @param timeout_ticks 最长等待 tick 数
     * @return ESP_OK 收到事件，ESP_ERR_TIMEOUT 等待超时，ESP_ERR_INVALID_STATE 队列未创建
     */
    esp_err_t audio_service_receive_voice_event(
        audio_service_voice_event_t *event,
        TickType_t timeout_ticks);

    /**
     * @brief 接收指定代次的流式语音 PCM
     * @param generation 目标非零会话代次
     * @param buffer PCM 输出缓冲区
     * @param buffer_size 缓冲区容量，通常为 1024 字节
     * @param bytes_received 实际接收字节数
     * @param timeout_ticks 最长等待 tick 数
     * @return ESP_OK 收到数据，ESP_ERR_TIMEOUT 暂无数据，ESP_ERR_INVALID_STATE 代次失效
     */
    esp_err_t audio_service_receive_voice_pcm(
        uint32_t generation,
        void *buffer,
        size_t buffer_size,
        size_t *bytes_received,
        TickType_t timeout_ticks);

    /**
     * @brief 查询语音采集所需音频资源与预分配 DMA 是否可用
     * @return true audio_task、固定提示音和 ES8311 TX/RX 均就绪且没有恢复故障
     */
    bool audio_service_voice_capture_ready(void);

    /**
     * @brief 请求 audio_task 有界停止并退出
     * @param timeout_ticks 等待队列空间的有限 tick 数
     * @return ESP_OK 已入队，ESP_ERR_INVALID_STATE 队列未创建，ESP_ERR_TIMEOUT 队列满
     */
    esp_err_t audio_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 查询 audio_task 是否已完成本轮硬件资源初始化尝试
     * @return true 表示 I2S DMA 已成功保留或失败结果已落定，false 表示仍在等待
     */
    bool audio_service_startup_complete(void);

    /**
     * @brief 进入统一服务框架拥有的 audio_task 循环
     * @details 收到 stop 后关闭 PA、释放 codec/I2S 并返回，由 legbot_services 回收任务。
     * @return ESP_OK 已安全退出，其他值表示最终清理或 terminal 状态交付失败
     */
    esp_err_t audio_service_run(void);

    /**
     * @brief 获取音频运行计时与栈高水位快照
     * @param metrics 输出指标
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效
     */
    esp_err_t audio_service_metrics_snapshot(audio_service_metrics_t *metrics);

    /**
     * @brief 在 state queue 创建后补发启动阶段的资源判定
     * @return ESP_OK 已发布，其他值表示 state queue 不可用或已满
     */
    esp_err_t audio_service_publish_initial_state(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_AUDIO_SERVICE_H */
