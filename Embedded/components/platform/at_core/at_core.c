/**
 * @file     at_core.c
 * @brief    ML307R AT 单读者事务核心实现
 * @details  配置 UART2 与有界队列，由 modem_task 串行执行发送、增量解析、URC 分发、超时和敏感数据清理。
 * @author   ZHC
 * @date     2026-07-13
 */

#include "at_core.h"

#include <limits.h>
#include <string.h>

#include "bsp_resources.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "PLAT_AT_CORE";

/** AT 命令队列深度。 */
#define AT_CORE_COMMAND_QUEUE_DEPTH 4U
/** AT 公共结果队列深度。 */
#define AT_CORE_RESULT_QUEUE_DEPTH 4U
/** AT 独立 URC 队列深度。 */
#define AT_CORE_URC_QUEUE_DEPTH 12U
/** UART 接收驱动缓冲区大小。 */
#define AT_CORE_UART_RX_BUFFER_BYTES 2048
/** UART 发送驱动缓冲区大小。 */
#define AT_CORE_UART_TX_BUFFER_BYTES 512
/** owner 每次读取的栈内块大小。 */
#define AT_CORE_RX_CHUNK_BYTES 128U
/** 超时或取消后等待旧响应安全边界的最大时间。 */
#define AT_CORE_RESYNC_TIMEOUT_MS 1000U
/** 普通 AT 命令等待最后一个停止位物理发出的上限。 */
#define AT_CORE_TX_DONE_TIMEOUT_MS 500U
/** active 加全部排队事务所需的完成终态 outbox 深度。 */
#define AT_CORE_PENDING_RESULT_DEPTH (AT_CORE_COMMAND_QUEUE_DEPTH + 1U)
/** 按 URC 类别保留的关键 latest-value 槽位数量。 */
#define AT_CORE_PENDING_URC_SLOT_COUNT 7U

typedef struct
{
    legbot_at_result_t result; /**< 尚未成功写入目标完成队列的终态。 */
    QueueHandle_t queue;       /**< 终态原目标队列。 */
} at_core_pending_result_t;

/** AT 命令按值队列。 */
static QueueHandle_t s_command_queue;
/** AT 公共完成结果队列。 */
static QueueHandle_t s_result_queue;
/** AT URC 独立队列。 */
static QueueHandle_t s_urc_queue;
/** 当前唯一 active transaction。 */
static legbot_at_transaction_t s_active;
/** 当前事务是否有效。 */
static bool s_active_valid;
/** 当前事务活动期间收到的 UART 字节饱和计数。 */
static uint32_t s_active_rx_byte_count;
/** 当前事务已累计的有界响应。 */
static char s_response[LEGBOT_AT_RESPONSE_MAX_LENGTH];
/** 当前响应有效长度。 */
static size_t s_response_length;
/** 是否正在按事务声明的精确长度接收原始响应。 */
static bool s_raw_response_active;
/** 原始响应尚待接收的字节数。 */
static size_t s_raw_response_remaining;
/** 匹配前缀后是否还需跳过 framing 的 LF。 */
static bool s_raw_skip_framing_lf;
/** 当前前缀行内已遇到的原始响应分隔逗号数。 */
static uint8_t s_raw_inline_delimiters_seen;
/** 当前增量解析行。 */
static char s_line[LEGBOT_AT_LINE_MAX_LENGTH];
/** 当前行有效长度。 */
static size_t s_line_length;
/** 超长输入是否正在丢弃到换行同步点。 */
static bool s_discard_until_lf;
/** 上一输入字节是否为回车。 */
static bool s_last_was_cr;
/** URC 发布序号。 */
static uint32_t s_urc_sequence;
/** AT 诊断饱和快照。 */
static at_core_diagnostics_t s_diagnostics;
/** 完成队列暂满时保留的有界终态 FIFO。 */
static at_core_pending_result_t s_pending_results[AT_CORE_PENDING_RESULT_DEPTH];
/** pending result FIFO 当前数量。 */
static size_t s_pending_result_count;
/** 关键 URC 队列满时按类别保留的 latest-value。 */
static legbot_at_urc_t s_pending_urcs[AT_CORE_PENDING_URC_SLOT_COUNT];
/** 关键 URC 类别槽是否有效。 */
static bool s_pending_urc_valid[AT_CORE_PENDING_URC_SLOT_COUNT];
/** 超时/取消后是否正在丢弃旧事务尾部并等待安全同步点。 */
static bool s_resync_active;
/** raw 事务中断后仍需丢弃的显式长度字节数。 */
static size_t s_resync_raw_remaining;
/** 重同步最长绝对截止时间。 */
static TickType_t s_resync_deadline_ticks;
/** 当前事务开始向 UART 驱动提交发送的单调时刻。 */
static TickType_t s_active_started_ticks;
/** 最近一次重同步开始的单调时刻。 */
static TickType_t s_resync_started_ticks;
/** 最近一次重同步开始时的 UART RX 累计值。 */
static uint32_t s_resync_started_rx_count;
/** 触发最近一次重同步的 AT 请求 ID。 */
static uint32_t s_resync_request_id;
/** AT 核心初始化状态。 */
static bool s_initialized;
/** UART2 当前运行时波特率。 */
static uint32_t s_baud_rate;

static esp_err_t at_core_configure_uart(void);
static void cleanup_queues(void);
static void secure_clear(void *buffer, size_t length);
static void saturating_increment(uint32_t *value);
static void saturating_add(uint32_t *value, size_t increment);
static void record_error(const char *code);
static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks);
static const char *result_status_text(at_core_result_status_t status);
static const char *diagnostic_command(const legbot_at_transaction_t *transaction);
static bool is_known_urc(const char *line);
static bool is_prefix_match(const char *line, const char *prefix);
static int pending_urc_slot(const char *line);
static void retry_pending_urcs(void);
static void publish_urc(const char *line);
static void append_response_line(const char *line);
static void retry_pending_results(void);
static bool queue_result(const legbot_at_result_t *result, QueueHandle_t queue);
static void publish_queued_terminal(const legbot_at_transaction_t *transaction,
                                    at_core_result_status_t status);
static void begin_resync(TickType_t now_ticks);
static void finish_resync(void);
static void publish_result(at_core_result_status_t status);
static void clear_active(void);
static void abort_command_queue(at_core_result_status_t status);
static esp_err_t start_next_transaction(TickType_t now_ticks);
static void handle_prompt(void);
static void handle_line(char *line, bool skip_framing_lf);
static esp_err_t write_active_command(void);

esp_err_t at_core_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }
    esp_err_t err = at_core_configure_uart();
    if (err != ESP_OK)
    {
        return err;
    }
    /* AT 队列只在 task 上下文访问，按值大载荷放入 PSRAM，保留内部 DMA 给 PHY 与 I2S。 */
    s_command_queue = xQueueCreateWithCaps(
        AT_CORE_COMMAND_QUEUE_DEPTH,
        sizeof(legbot_at_transaction_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_result_queue = xQueueCreateWithCaps(
        AT_CORE_RESULT_QUEUE_DEPTH,
        sizeof(legbot_at_result_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_urc_queue = xQueueCreateWithCaps(
        AT_CORE_URC_QUEUE_DEPTH,
        sizeof(legbot_at_urc_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_command_queue == NULL || s_result_queue == NULL || s_urc_queue == NULL)
    {
        cleanup_queues();
        (void)uart_driver_delete(LEGBOT_BSP_ML307R_UART_PORT);
        return ESP_ERR_NO_MEM;
    }
    memset(&s_diagnostics, 0, sizeof(s_diagnostics));
    record_error("MODEM_OK");
    s_baud_rate = LEGBOT_BSP_ML307R_UART_BAUD;
    s_initialized = true;
    ESP_LOGI(TAG,
             "AT 核心已配置：UART%d 固定 115200/8N1，TX=IO%d、RX=IO%d，reader 归固定 modem_task 所有",
             (int)LEGBOT_BSP_ML307R_UART_PORT,
             (int)LEGBOT_BSP_ML307R_TX_GPIO,
             (int)LEGBOT_BSP_ML307R_RX_GPIO);
    return ESP_OK;
}

bool at_core_is_initialized(void)
{
    return s_initialized;
}

uart_port_t at_core_uart_port(void)
{
    return LEGBOT_BSP_ML307R_UART_PORT;
}

esp_err_t at_core_submit(const legbot_at_transaction_t *transaction,
                         TickType_t timeout_ticks)
{
    if (!s_initialized || s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (transaction == NULL || transaction->request_id == 0U ||
        transaction->phase < AT_CORE_PHASE_COMMAND ||
        transaction->phase > AT_CORE_PHASE_FINAL ||
        transaction->deadline_ticks == 0U ||
        memchr(transaction->command, '\0', sizeof(transaction->command)) == NULL ||
        transaction->command[0] == '\0' ||
        memchr(transaction->expected_prefix, '\0',
               sizeof(transaction->expected_prefix)) == NULL ||
        transaction->data_length > LEGBOT_AT_DATA_MAX_LENGTH ||
        transaction->raw_response_length > LEGBOT_AT_RESPONSE_MAX_LENGTH ||
        (transaction->raw_response_length != 0U &&
         transaction->expected_prefix[0] == '\0') ||
        (transaction->raw_response_inline_delimiters != 0U &&
         transaction->raw_response_length == 0U) ||
        (transaction->phase == AT_CORE_PHASE_DATA_PROMPT &&
         transaction->data_length == 0U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xQueueSend(s_command_queue, transaction, timeout_ticks) != pdTRUE)
    {
        saturating_increment(&s_diagnostics.command_queue_full_count);
        record_error("MODEM_AT_QUEUE_FULL");
        ESP_LOGW(TAG, "AT 命令队列已满，请求=%lu", (unsigned long)transaction->request_id);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t at_core_owner_poll(TickType_t now_ticks, TickType_t read_wait_ticks)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    retry_pending_results();
    retry_pending_urcs();
    if (s_resync_active && tick_reached(now_ticks, s_resync_deadline_ticks))
    {
        finish_resync();
    }
    if (!s_active_valid && s_pending_result_count == 0U && !s_resync_active)
    {
        esp_err_t start_error = start_next_transaction(now_ticks);
        if (start_error != ESP_OK && start_error != ESP_ERR_NOT_FOUND)
        {
            return start_error;
        }
    }
    uint8_t bytes[AT_CORE_RX_CHUNK_BYTES];
    int received = uart_read_bytes(LEGBOT_BSP_ML307R_UART_PORT,
                                   bytes,
                                   sizeof(bytes),
                                   read_wait_ticks);
    const TickType_t refreshed_ticks = xTaskGetTickCount();
    if (tick_reached(refreshed_ticks, now_ticks))
    {
        now_ticks = refreshed_ticks;
    }
    if (received < 0)
    {
        saturating_increment(&s_diagnostics.rx_error_count);
        record_error("MODEM_UART_READ");
        if (s_active_valid)
        {
            publish_result(AT_CORE_RESULT_UART_ERROR);
        }
        ESP_LOGE(TAG,
                 "UART2 读取失败，活动=%u，请求=%lu，重同步=%u，全局 RX=%lu，驱动 TX=%lu，读取错误=%lu",
                 s_active_valid ? 1U : 0U,
                 (unsigned long)(s_active_valid ? s_active.request_id : 0U),
                 s_resync_active ? 1U : 0U,
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned long)s_diagnostics.tx_byte_count,
                 (unsigned long)s_diagnostics.rx_error_count);
        return ESP_FAIL;
    }
    if (received > 0)
    {
        esp_err_t feed_error = at_core_owner_feed(bytes, (size_t)received, now_ticks);
        if (feed_error != ESP_OK)
        {
            return feed_error;
        }
    }
    if (s_active_valid && tick_reached(now_ticks, s_active.deadline_ticks))
    {
        record_error("MODEM_AT_TIMEOUT");
        begin_resync(now_ticks);
        publish_result(AT_CORE_RESULT_TIMEOUT);
    }
    if (!s_active_valid && s_pending_result_count == 0U && !s_resync_active)
    {
        (void)start_next_transaction(now_ticks);
    }
    return ESP_OK;
}

esp_err_t at_core_owner_feed(const uint8_t *bytes,
                             size_t length,
                             TickType_t now_ticks)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (bytes == NULL && length != 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    saturating_add(&s_diagnostics.rx_byte_count, length);
    if (length > 0U)
    {
        s_diagnostics.last_rx_ticks = now_ticks;
    }
    for (size_t index = 0; index < length; ++index)
    {
        if (s_active_valid)
        {
            saturating_increment(&s_active_rx_byte_count);
        }
        const uint8_t byte = bytes[index];
        if (s_active_valid && tick_reached(now_ticks, s_active.deadline_ticks))
        {
            record_error("MODEM_AT_TIMEOUT");
            begin_resync(now_ticks);
            publish_result(AT_CORE_RESULT_TIMEOUT);
        }
        if (s_resync_active && s_resync_raw_remaining > 0U)
        {
            --s_resync_raw_remaining;
            continue;
        }
        if (s_active_valid && s_raw_response_active)
        {
            if (s_raw_skip_framing_lf)
            {
                s_raw_skip_framing_lf = false;
                s_last_was_cr = false;
                if (byte == '\n')
                {
                    continue;
                }
            }
            if (s_response_length >= sizeof(s_response) ||
                s_raw_response_remaining == 0U)
            {
                record_error("MODEM_RESPONSE_OVERFLOW");
                begin_resync(now_ticks);
                publish_result(AT_CORE_RESULT_RESPONSE_OVERFLOW);
                continue;
            }
            s_response[s_response_length++] = (char)byte;
            --s_raw_response_remaining;
            if (s_raw_response_remaining == 0U)
            {
                s_raw_response_active = false;
                s_line_length = 0U;
                s_last_was_cr = false;
            }
            continue;
        }
        if (s_discard_until_lf)
        {
            if (byte == '\n')
            {
                s_discard_until_lf = false;
                s_line_length = 0U;
                s_last_was_cr = false;
            }
            continue;
        }
        if (byte == '>' && s_line_length == 0U && s_active_valid &&
            s_active.phase == AT_CORE_PHASE_DATA_PROMPT)
        {
            handle_prompt();
            continue;
        }
        if (byte == '\r' || byte == '\n')
        {
            if (s_line_length > 0U &&
                (byte == '\n' || !s_last_was_cr))
            {
                s_line[s_line_length] = '\0';
                handle_line(s_line, byte == '\r');
                s_line_length = 0U;
            }
            s_raw_inline_delimiters_seen = 0U;
            s_last_was_cr = byte == '\r';
            continue;
        }
        s_last_was_cr = false;
        if (s_line_length + 1U >= sizeof(s_line))
        {
            s_discard_until_lf = true;
            s_line_length = 0U;
            saturating_increment(&s_diagnostics.line_overflow_count);
            record_error("MODEM_LINE_OVERFLOW");
            ESP_LOGW(TAG, "AT 输入行过长，已丢弃到安全同步点");
            if (s_active_valid)
            {
                begin_resync(now_ticks);
                publish_result(AT_CORE_RESULT_RESPONSE_OVERFLOW);
            }
            continue;
        }
        s_line[s_line_length++] = (char)byte;
        if (s_active_valid &&
            s_active.raw_response_inline_delimiters != 0U)
        {
            s_line[s_line_length] = '\0';
            if (is_prefix_match(s_line, s_active.expected_prefix) &&
                byte == ',')
            {
                ++s_raw_inline_delimiters_seen;
                if (s_raw_inline_delimiters_seen ==
                    s_active.raw_response_inline_delimiters)
                {
                    memset(s_response, 0, sizeof(s_response));
                    s_response_length = 0U;
                    s_raw_response_active = true;
                    s_raw_response_remaining =
                        s_active.raw_response_length;
                    s_raw_skip_framing_lf = false;
                    s_raw_inline_delimiters_seen = 0U;
                    s_line_length = 0U;
                    s_last_was_cr = false;
                }
            }
        }
    }
    return ESP_OK;
}

esp_err_t at_core_receive_result(legbot_at_result_t *result,
                                 TickType_t timeout_ticks)
{
    if (!s_initialized || s_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return xQueueReceive(s_result_queue, result, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t at_core_receive_urc(legbot_at_urc_t *urc,
                              TickType_t timeout_ticks)
{
    if (!s_initialized || s_urc_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (urc == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return xQueueReceive(s_urc_queue, urc, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t at_core_cancel(uint32_t request_id)
{
    if (!s_initialized || request_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_active_valid || s_active.request_id != request_id)
    {
        return ESP_ERR_NOT_FOUND;
    }
    record_error("MODEM_AT_CANCELLED");
    begin_resync(xTaskGetTickCount());
    publish_result(AT_CORE_RESULT_CANCELLED);
    return ESP_OK;
}

void at_core_abort_all(bool modem_restarted)
{
    if (!s_initialized)
    {
        return;
    }
    const at_core_result_status_t status = modem_restarted
                                               ? AT_CORE_RESULT_MODEM_RESTARTED
                                               : AT_CORE_RESULT_CANCELLED;
    ESP_LOGW(TAG,
             "AT 核心作废全部事务，原因=%s，活动=%u，请求=%lu，命令=%s，重同步=%u，全局 RX=%lu，驱动 TX=%lu",
             modem_restarted ? "模组重启" : "owner 取消",
             s_active_valid ? 1U : 0U,
             (unsigned long)(s_active_valid ? s_active.request_id : 0U),
             s_active_valid ? diagnostic_command(&s_active) : "无",
             s_resync_active ? 1U : 0U,
             (unsigned long)s_diagnostics.rx_byte_count,
             (unsigned long)s_diagnostics.tx_byte_count);
    abort_command_queue(status);
    if (s_active_valid)
    {
        record_error(modem_restarted ? "MODEM_RESTARTED" : "MODEM_AT_CANCELLED");
        if (!modem_restarted)
        {
            begin_resync(xTaskGetTickCount());
        }
        publish_result(status);
    }
    if (modem_restarted)
    {
        finish_resync();
    }
    s_line_length = 0U;
    s_discard_until_lf = false;
    s_last_was_cr = false;
}

esp_err_t at_core_diagnostics_snapshot(at_core_diagnostics_t *diagnostics)
{
    if (diagnostics == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *diagnostics = s_diagnostics;
    return ESP_OK;
}

esp_err_t at_core_update_baud_rate(uint32_t baud_rate)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (baud_rate == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_active_valid)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = uart_wait_tx_done(
        LEGBOT_BSP_ML307R_UART_PORT,
        pdMS_TO_TICKS(AT_CORE_TX_DONE_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        record_error("MODEM_UART_TX_TIMEOUT");
        return err;
    }
    err = uart_flush_input(LEGBOT_BSP_ML307R_UART_PORT);
    if (err != ESP_OK)
    {
        record_error("MODEM_UART_FLUSH");
        return err;
    }
    err = uart_set_baudrate(LEGBOT_BSP_ML307R_UART_PORT, baud_rate);
    if (err != ESP_OK)
    {
        record_error("MODEM_UART_BAUD");
        return err;
    }
    finish_resync();
    s_baud_rate = baud_rate;
    ESP_LOGI(TAG, "UART2 已切换波特率=%lu", (unsigned long)baud_rate);
    return ESP_OK;
}

uint32_t at_core_baud_rate(void)
{
    return s_initialized ? s_baud_rate : 0U;
}

static esp_err_t at_core_configure_uart(void)
{
    const uart_config_t config = {
        .baud_rate = LEGBOT_BSP_ML307R_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(LEGBOT_BSP_ML307R_UART_PORT, &config);
    if (err != ESP_OK)
    {
        return err;
    }
    err = uart_set_pin(LEGBOT_BSP_ML307R_UART_PORT,
                       LEGBOT_BSP_ML307R_TX_GPIO,
                       LEGBOT_BSP_ML307R_RX_GPIO,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        return err;
    }
    return uart_driver_install(LEGBOT_BSP_ML307R_UART_PORT,
                               AT_CORE_UART_RX_BUFFER_BYTES,
                               AT_CORE_UART_TX_BUFFER_BYTES,
                               0,
                               NULL,
                               0);
}

static void cleanup_queues(void)
{
    if (s_command_queue != NULL)
    {
        vQueueDeleteWithCaps(s_command_queue);
        s_command_queue = NULL;
    }
    if (s_result_queue != NULL)
    {
        vQueueDeleteWithCaps(s_result_queue);
        s_result_queue = NULL;
    }
    if (s_urc_queue != NULL)
    {
        vQueueDeleteWithCaps(s_urc_queue);
        s_urc_queue = NULL;
    }
}

static void secure_clear(void *buffer, size_t length)
{
    volatile uint8_t *bytes = (volatile uint8_t *)buffer;
    while (length-- > 0U)
    {
        *bytes++ = 0U;
    }
}

static void saturating_increment(uint32_t *value)
{
    if (*value < UINT32_MAX)
    {
        ++(*value);
    }
}

static void saturating_add(uint32_t *value, size_t increment)
{
    if (increment >= UINT32_MAX ||
        *value > UINT32_MAX - (uint32_t)increment)
    {
        *value = UINT32_MAX;
        return;
    }
    *value += (uint32_t)increment;
}

static void record_error(const char *code)
{
    size_t length = strnlen(code, sizeof(s_diagnostics.last_error));
    if (length >= sizeof(s_diagnostics.last_error))
    {
        code = "MODEM_INTERNAL";
        length = strlen(code);
    }
    memset(s_diagnostics.last_error, 0, sizeof(s_diagnostics.last_error));
    memcpy(s_diagnostics.last_error, code, length);
}

static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks)
{
    return (int32_t)(now_ticks - deadline_ticks) >= 0;
}

static const char *result_status_text(at_core_result_status_t status)
{
    switch (status)
    {
    case AT_CORE_RESULT_OK:
        return "OK";
    case AT_CORE_RESULT_ERROR:
        return "ERROR";
    case AT_CORE_RESULT_CME_ERROR:
        return "CME_ERROR";
    case AT_CORE_RESULT_TIMEOUT:
        return "TIMEOUT";
    case AT_CORE_RESULT_CANCELLED:
        return "CANCELLED";
    case AT_CORE_RESULT_UART_ERROR:
        return "UART_ERROR";
    case AT_CORE_RESULT_MODEM_RESTARTED:
        return "MODEM_RESTARTED";
    case AT_CORE_RESULT_RESPONSE_OVERFLOW:
        return "RESPONSE_OVERFLOW";
    case AT_CORE_RESULT_PROMPT:
        return "PROMPT";
    default:
        return "UNKNOWN";
    }
}

static const char *diagnostic_command(const legbot_at_transaction_t *transaction)
{
    if (transaction == NULL)
    {
        return "无";
    }
    return transaction->sensitive ? "<敏感命令>" : transaction->command;
}

static bool is_known_urc(const char *line)
{
    static const char *const prefixes[] = {
        "+MATREADY", "+CEREG:", "+CREG:", "+CGREG:",
        "+CPIN:", "+MIPCALL:", "+MHTTPURC:"};
    for (size_t index = 0; index < sizeof(prefixes) / sizeof(prefixes[0]); ++index)
    {
        if (is_prefix_match(line, prefixes[index]))
        {
            return true;
        }
    }
    return false;
}

static int pending_urc_slot(const char *line)
{
    static const char *const prefixes[AT_CORE_PENDING_URC_SLOT_COUNT] = {
        "+MATREADY", "+CEREG:", "+CREG:", "+CGREG:",
        "+CPIN:", "+MIPCALL:", "+MHTTPURC:"};
    for (size_t index = 0U; index < AT_CORE_PENDING_URC_SLOT_COUNT; ++index)
    {
        if (is_prefix_match(line, prefixes[index]))
        {
            return (int)index;
        }
    }
    return -1;
}

static void retry_pending_urcs(void)
{
    if (s_urc_queue == NULL)
    {
        return;
    }
    for (size_t index = 0U; index < AT_CORE_PENDING_URC_SLOT_COUNT; ++index)
    {
        if (!s_pending_urc_valid[index])
        {
            continue;
        }
        if (xQueueSend(s_urc_queue, &s_pending_urcs[index], 0) != pdTRUE)
        {
            return;
        }
        secure_clear(&s_pending_urcs[index], sizeof(s_pending_urcs[index]));
        s_pending_urc_valid[index] = false;
    }
}

static bool is_prefix_match(const char *line, const char *prefix)
{
    return prefix[0] != '\0' && strncmp(line, prefix, strlen(prefix)) == 0;
}

static void publish_urc(const char *line)
{
    legbot_at_urc_t urc = {0};
    size_t length = strnlen(line, sizeof(urc.line));
    if (length >= sizeof(urc.line))
    {
        saturating_increment(&s_diagnostics.urc_overflow_count);
        record_error("MODEM_URC_OVERFLOW");
        return;
    }
    saturating_increment(&s_urc_sequence);
    urc.sequence = s_urc_sequence;
    memcpy(urc.line, line, length + 1U);
    if (xQueueSend(s_urc_queue, &urc, 0) != pdTRUE)
    {
        saturating_increment(&s_diagnostics.urc_overflow_count);
        record_error("MODEM_URC_OVERFLOW");
        const int slot = pending_urc_slot(line);
        if (slot >= 0)
        {
            /* HTTP 终态保留首条，其余状态 URC 合并为最新值。 */
            if (slot != (int)(AT_CORE_PENDING_URC_SLOT_COUNT - 1U) ||
                !s_pending_urc_valid[slot])
            {
                s_pending_urcs[slot] = urc;
                s_pending_urc_valid[slot] = true;
            }
        }
        ESP_LOGW(TAG, "URC 队列已满，重复状态可被合并");
    }
}

static void append_response_line(const char *line)
{
    const size_t line_length = strlen(line);
    const size_t separator_length = s_response_length == 0U ? 0U : 2U;
    if (s_response_length + separator_length + line_length + 1U > sizeof(s_response))
    {
        record_error("MODEM_RESPONSE_OVERFLOW");
        begin_resync(xTaskGetTickCount());
        publish_result(AT_CORE_RESULT_RESPONSE_OVERFLOW);
        return;
    }
    if (separator_length != 0U)
    {
        memcpy(&s_response[s_response_length], "\r\n", 2U);
        s_response_length += 2U;
    }
    memcpy(&s_response[s_response_length], line, line_length);
    s_response_length += line_length;
    s_response[s_response_length] = '\0';
}

static void retry_pending_results(void)
{
    while (s_pending_result_count > 0U)
    {
        at_core_pending_result_t *pending = &s_pending_results[0];
        if (pending->queue == NULL ||
            xQueueSend(pending->queue, &pending->result, 0) != pdTRUE)
        {
            saturating_increment(&s_diagnostics.result_queue_full_count);
            record_error("MODEM_RESULT_QUEUE_FULL");
            return;
        }
        secure_clear(pending, sizeof(*pending));
        --s_pending_result_count;
        if (s_pending_result_count > 0U)
        {
            memmove(&s_pending_results[0],
                    &s_pending_results[1],
                    s_pending_result_count * sizeof(s_pending_results[0]));
            secure_clear(&s_pending_results[s_pending_result_count],
                         sizeof(s_pending_results[0]));
        }
    }
}

static bool queue_result(const legbot_at_result_t *result, QueueHandle_t queue)
{
    if (queue != NULL && xQueueSend(queue, result, 0) == pdTRUE)
    {
        return true;
    }
    saturating_increment(&s_diagnostics.result_queue_full_count);
    record_error("MODEM_RESULT_QUEUE_FULL");
    if (s_pending_result_count >= AT_CORE_PENDING_RESULT_DEPTH)
    {
        ESP_LOGE(TAG, "AT 终态 outbox 已满，请求=%lu",
                 (unsigned long)result->request_id);
        return false;
    }
    s_pending_results[s_pending_result_count].result = *result;
    s_pending_results[s_pending_result_count].queue = queue;
    ++s_pending_result_count;
    return false;
}

static void publish_queued_terminal(const legbot_at_transaction_t *transaction,
                                    at_core_result_status_t status)
{
    legbot_at_result_t result = {
        .request_id = transaction->request_id,
        .status = status,
    };
    QueueHandle_t queue = transaction->completion_queue != NULL
                              ? transaction->completion_queue
                              : s_result_queue;
    (void)queue_result(&result, queue);
    secure_clear(&result, sizeof(result));
}

static void begin_resync(TickType_t now_ticks)
{
    if (!s_resync_active)
    {
        s_resync_raw_remaining = s_raw_response_active
                                     ? s_raw_response_remaining
                                     : 0U;
        s_resync_started_ticks = now_ticks;
        s_resync_started_rx_count = s_diagnostics.rx_byte_count;
        s_resync_request_id = s_active_valid ? s_active.request_id : 0U;
        saturating_increment(&s_diagnostics.resync_count);
        ESP_LOGW(TAG,
                 "AT 重同步开始，请求=%lu，命令=%s，事务 RX=%lu，全局 RX=%lu，行长度=%u，丢弃到换行=%u，raw 剩余=%u，窗口=%u ms，核心错误=%s",
                 (unsigned long)s_resync_request_id,
                 s_active_valid ? diagnostic_command(&s_active) : "无",
                 (unsigned long)s_active_rx_byte_count,
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned)s_line_length,
                 s_discard_until_lf ? 1U : 0U,
                 (unsigned)s_resync_raw_remaining,
                 AT_CORE_RESYNC_TIMEOUT_MS,
                 s_diagnostics.last_error);
        if (s_active_valid &&
            strncmp(s_active.command, "AT+CFUN", 7U) == 0 &&
            s_line_length > 0U)
        {
            ESP_LOGW(TAG,
                     "CFUN 未闭合行诊断，请求=%lu，长度=%u，首字节=0x%02X，尾字节=0x%02X",
                     (unsigned long)s_active.request_id,
                     (unsigned)s_line_length,
                     (unsigned)(uint8_t)s_line[0],
                     (unsigned)(uint8_t)s_line[s_line_length - 1U]);
        }
    }
    s_resync_active = true;
    s_resync_deadline_ticks = now_ticks + pdMS_TO_TICKS(AT_CORE_RESYNC_TIMEOUT_MS);
}

static void finish_resync(void)
{
    if (s_resync_active)
    {
        const TickType_t now_ticks = xTaskGetTickCount();
        ESP_LOGI(TAG,
                 "AT 重同步结束，请求=%lu，耗时=%lu ms，窗口内 RX=%lu，全局 RX=%lu，最后 RX tick=%lu",
                 (unsigned long)s_resync_request_id,
                 (unsigned long)pdTICKS_TO_MS(now_ticks -
                                              s_resync_started_ticks),
                 (unsigned long)(s_diagnostics.rx_byte_count -
                                 s_resync_started_rx_count),
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned long)s_diagnostics.last_rx_ticks);
    }
    s_resync_active = false;
    s_resync_raw_remaining = 0U;
    s_resync_deadline_ticks = 0U;
    s_resync_started_ticks = 0U;
    s_resync_started_rx_count = 0U;
    s_resync_request_id = 0U;
    s_discard_until_lf = false;
    s_line_length = 0U;
    s_last_was_cr = false;
}

static void publish_result(at_core_result_status_t status)
{
    if (!s_active_valid)
    {
        return;
    }
    if (status == AT_CORE_RESULT_TIMEOUT)
    {
        saturating_increment(&s_diagnostics.timeout_count);
    }
    const TickType_t now_ticks = xTaskGetTickCount();
    const uint32_t elapsed_ms =
        s_active_started_ticks == 0U
            ? 0U
            : pdTICKS_TO_MS(now_ticks - s_active_started_ticks);
    if (status == AT_CORE_RESULT_OK)
    {
        ESP_LOGI(TAG,
                 "AT 事务终态，请求=%lu，命令=%s，状态=%s(%u)，耗时=%lu ms，事务 RX=%lu，响应=%u，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu，重同步=%u",
                 (unsigned long)s_active.request_id,
                 diagnostic_command(&s_active),
                 result_status_text(status),
                 (unsigned)status,
                 (unsigned long)elapsed_ms,
                 (unsigned long)s_active_rx_byte_count,
                 (unsigned)s_response_length,
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned long)s_diagnostics.tx_byte_count,
                 (unsigned long)s_diagnostics.last_rx_ticks,
                 s_resync_active ? 1U : 0U);
    }
    else
    {
        ESP_LOGW(TAG,
                 "AT 事务终态，请求=%lu，命令=%s，状态=%s(%u)，耗时=%lu ms，事务 RX=%lu，响应=%u，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu，重同步=%u，行长度=%u，丢弃到换行=%u，核心错误=%s",
                 (unsigned long)s_active.request_id,
                 diagnostic_command(&s_active),
                 result_status_text(status),
                 (unsigned)status,
                 (unsigned long)elapsed_ms,
                 (unsigned long)s_active_rx_byte_count,
                 (unsigned)s_response_length,
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned long)s_diagnostics.tx_byte_count,
                 (unsigned long)s_diagnostics.last_rx_ticks,
                 s_resync_active ? 1U : 0U,
                 (unsigned)s_line_length,
                 s_discard_until_lf ? 1U : 0U,
                 s_diagnostics.last_error);
    }
    legbot_at_result_t result = {
        .request_id = s_active.request_id,
        .status = status,
        .response_length = (uint16_t)s_response_length,
        .rx_byte_count = s_active_rx_byte_count,
    };
    memcpy(result.response, s_response, s_response_length);
    QueueHandle_t queue = s_active.completion_queue != NULL
                              ? s_active.completion_queue
                              : s_result_queue;
    if (!queue_result(&result, queue))
    {
        ESP_LOGE(TAG, "AT 完成队列已满，请求=%lu", (unsigned long)s_active.request_id);
    }
    secure_clear(&result, sizeof(result));
    clear_active();
}

static void clear_active(void)
{
    if (s_active.sensitive)
    {
        secure_clear(&s_active, sizeof(s_active));
        secure_clear(s_response, sizeof(s_response));
        secure_clear(s_line, sizeof(s_line));
    }
    else
    {
        memset(&s_active, 0, sizeof(s_active));
        memset(s_response, 0, sizeof(s_response));
        memset(s_line, 0, sizeof(s_line));
    }
    s_response_length = 0U;
    s_raw_response_active = false;
    s_raw_response_remaining = 0U;
    s_raw_skip_framing_lf = false;
    s_raw_inline_delimiters_seen = 0U;
    s_line_length = 0U;
    s_last_was_cr = false;
    s_active_rx_byte_count = 0U;
    s_active_started_ticks = 0U;
    s_active_valid = false;
}

static void abort_command_queue(at_core_result_status_t status)
{
    if (s_command_queue == NULL)
    {
        return;
    }
    legbot_at_transaction_t transaction = {0};
    while (xQueueReceive(s_command_queue, &transaction, 0) == pdTRUE)
    {
        publish_queued_terminal(&transaction, status);
        secure_clear(&transaction, sizeof(transaction));
    }
    (void)xQueueReset(s_command_queue);
    secure_clear(&transaction, sizeof(transaction));
}

static esp_err_t start_next_transaction(TickType_t now_ticks)
{
    if (s_active_valid || s_pending_result_count > 0U || s_resync_active)
    {
        return ESP_OK;
    }
    if (s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    while (xQueueReceive(s_command_queue, &s_active, 0) == pdTRUE)
    {
        s_active_valid = true;
        s_active_rx_byte_count = 0U;
        s_response_length = 0U;
        memset(s_response, 0, sizeof(s_response));
        s_raw_response_active = false;
        s_raw_response_remaining = 0U;
        s_raw_skip_framing_lf = false;
        s_raw_inline_delimiters_seen = 0U;
        if (tick_reached(now_ticks, s_active.deadline_ticks))
        {
            record_error("MODEM_AT_TIMEOUT");
            publish_result(AT_CORE_RESULT_TIMEOUT);
            if (s_pending_result_count > 0U)
            {
                return ESP_OK;
            }
            continue;
        }
        if (s_active.response_timeout_ticks != 0U)
        {
            const TickType_t response_deadline =
                now_ticks + s_active.response_timeout_ticks;
            if (tick_reached(s_active.deadline_ticks, response_deadline))
            {
                s_active.deadline_ticks = response_deadline;
            }
        }
        return write_active_command();
    }
    return ESP_ERR_NOT_FOUND;
}

static void handle_prompt(void)
{
    if (!s_active_valid || s_active.phase != AT_CORE_PHASE_DATA_PROMPT)
    {
        return;
    }
    int written = uart_write_bytes(LEGBOT_BSP_ML307R_UART_PORT,
                                   s_active.data,
                                   s_active.data_length);
    if (written < 0 || (size_t)written != s_active.data_length)
    {
        record_error("MODEM_UART_WRITE");
        publish_result(AT_CORE_RESULT_UART_ERROR);
        return;
    }
    saturating_add(&s_diagnostics.tx_byte_count, s_active.data_length);
    s_diagnostics.last_tx_ticks = xTaskGetTickCount();
    s_active.phase = AT_CORE_PHASE_FINAL;
}

static void handle_line(char *line, bool skip_framing_lf)
{
    if (line[0] == '\0')
    {
        return;
    }
    const bool cme_error = is_prefix_match(line, "+CME ERROR");
    const bool terminal = strcmp(line, "OK") == 0 ||
                          strcmp(line, "ERROR") == 0 || cme_error;
    const bool unmatched_plus = line[0] == '+' && !cme_error &&
                                (!s_active_valid ||
                                 (s_active.expected_prefix[0] != '\0' &&
                                  !is_prefix_match(line, s_active.expected_prefix)));
    const bool urc = is_known_urc(line) || unmatched_plus;
    if (s_active_valid &&
        strncmp(s_active.command, "AT+CFUN", 7U) == 0)
    {
        const size_t diagnostic_length = strlen(line);
        if (strncmp(line, "+CFUN:", 6U) == 0 ||
            strcmp(line, "OK") == 0 || strcmp(line, "ERROR") == 0)
        {
            ESP_LOGI(TAG,
                     "CFUN 响应行，请求=%lu，长度=%u，内容=%s",
                     (unsigned long)s_active.request_id,
                     (unsigned)diagnostic_length,
                     line);
        }
        else if (diagnostic_length > 0U)
        {
            ESP_LOGI(TAG,
                     "CFUN 并发 URC 诊断，请求=%lu，长度=%u，首字节=0x%02X，尾字节=0x%02X",
                     (unsigned long)s_active.request_id,
                     (unsigned)diagnostic_length,
                     (unsigned)(uint8_t)line[0],
                     (unsigned)(uint8_t)line[diagnostic_length - 1U]);
        }
    }
    if (strcmp(line, "+MATREADY") == 0)
    {
        ESP_LOGW(TAG,
                 "UART2 观察到 MATREADY，活动=%u，请求=%lu，命令=%s，重同步=%u，全局 RX=%lu，最后 TX tick=%lu",
                 s_active_valid ? 1U : 0U,
                 (unsigned long)(s_active_valid ? s_active.request_id : 0U),
                 s_active_valid ? diagnostic_command(&s_active) : "无",
                 s_resync_active ? 1U : 0U,
                 (unsigned long)s_diagnostics.rx_byte_count,
                 (unsigned long)s_diagnostics.last_tx_ticks);
    }
    if (s_resync_active)
    {
        if (urc)
        {
            publish_urc(line);
        }
        if (terminal)
        {
            finish_resync();
        }
        return;
    }
    if (urc)
    {
        publish_urc(line);
        if (strcmp(line, "+MATREADY") == 0 && s_active_valid)
        {
            record_error("MODEM_RESTARTED");
            publish_result(AT_CORE_RESULT_MODEM_RESTARTED);
            return;
        }
    }
    if (!s_active_valid)
    {
        return;
    }
    if (strcmp(line, "OK") == 0)
    {
        publish_result(AT_CORE_RESULT_OK);
    }
    else if (strcmp(line, "ERROR") == 0)
    {
        record_error("MODEM_AT_ERROR");
        publish_result(AT_CORE_RESULT_ERROR);
    }
    else if (cme_error)
    {
        append_response_line(line);
        if (s_active_valid)
        {
            record_error("MODEM_AT_CME_ERROR");
            publish_result(AT_CORE_RESULT_CME_ERROR);
        }
    }
    else if (s_active.raw_response_length != 0U &&
             s_active.raw_response_inline_delimiters == 0U &&
             is_prefix_match(line, s_active.expected_prefix))
    {
        memset(s_response, 0, sizeof(s_response));
        s_response_length = 0U;
        s_raw_response_active = true;
        s_raw_response_remaining = s_active.raw_response_length;
        s_raw_skip_framing_lf = skip_framing_lf;
    }
    else if (!urc || is_prefix_match(line, s_active.expected_prefix))
    {
        append_response_line(line);
    }
}

static esp_err_t write_active_command(void)
{
    const TickType_t now_ticks = xTaskGetTickCount();
    s_active_started_ticks = now_ticks;
    saturating_increment(&s_diagnostics.transaction_count);
    const uint32_t deadline_remaining_ms =
        tick_reached(now_ticks, s_active.deadline_ticks)
            ? 0U
            : pdTICKS_TO_MS(s_active.deadline_ticks - now_ticks);
    ESP_LOGI(TAG,
             "AT 事务发送，请求=%lu，类别=%u，命令=%s，预期=%s，截止剩余=%lu ms，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu",
             (unsigned long)s_active.request_id,
             (unsigned)s_active.phase,
             diagnostic_command(&s_active),
             s_active.expected_prefix[0] == '\0'
                 ? "无"
                 : s_active.expected_prefix,
             (unsigned long)deadline_remaining_ms,
             (unsigned long)s_diagnostics.rx_byte_count,
             (unsigned long)s_diagnostics.tx_byte_count,
             (unsigned long)s_diagnostics.last_rx_ticks);
    const size_t command_length = strlen(s_active.command);
    int written = uart_write_bytes(LEGBOT_BSP_ML307R_UART_PORT,
                                   s_active.command,
                                   command_length);
    if (written < 0 || (size_t)written != command_length)
    {
        record_error("MODEM_UART_WRITE");
        publish_result(AT_CORE_RESULT_UART_ERROR);
        return ESP_FAIL;
    }
    saturating_add(&s_diagnostics.tx_byte_count, command_length);
    s_diagnostics.last_tx_ticks = xTaskGetTickCount();
    static const char ending[] = "\r\n";
    written = uart_write_bytes(LEGBOT_BSP_ML307R_UART_PORT,
                               ending,
                               sizeof(ending) - 1U);
    if (written != (int)(sizeof(ending) - 1U))
    {
        record_error("MODEM_UART_WRITE");
        publish_result(AT_CORE_RESULT_UART_ERROR);
        return ESP_FAIL;
    }
    saturating_add(&s_diagnostics.tx_byte_count, sizeof(ending) - 1U);
    s_diagnostics.last_tx_ticks = xTaskGetTickCount();
    return ESP_OK;
}
