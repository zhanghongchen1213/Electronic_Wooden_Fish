/**
 * @file     ui_manifest_projection.c
 * @brief    EWF 壳层静态标签表（无 exo/BLE 等遗留词）。
 * @author   ZHC
 * @date     2026-09-24
 */
#include "ui_manifest_projection.h"

static const char * const s_labels[] = {
    "木鱼",
    "心经",
    "统计",
    "设置",
    "音量",
    "亮度",
    "熄屏",
    "屏幕亮度",
    "自动熄屏",
    "低",
    "中",
    "高",
    "立即同步",
    "待同步",
    "同步中",
    "同步失败",
    "已同步",
    "今日敲击",
    "累计敲击",
    "次",
    "待校时",
    "本次诵读",
    "已完成",
    "本轮完成",
    "从头开始",
    "退出",
    "充电中 · 暂停敲击",
    "木鱼版本",
    "木鱼ID",
};

const char * const * ui_manifest_projection_static_labels(int *count)
{
    if (count) {
        *count = (int)(sizeof(s_labels) / sizeof(s_labels[0]));
    }
    return s_labels;
}
