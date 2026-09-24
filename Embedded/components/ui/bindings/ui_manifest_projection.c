/**
 * @file     ui_manifest_projection.c
 * @brief    EWF 壳层静态标签表（无 exo/BLE 等遗留词）。
 * @author   ZHC
 * @date     2026-09-24
 */
#include "ui_manifest_projection.h"

static const char * const s_labels[] = {
    "木鱼",
    "经文",
    "统计",
    "设置",
    "音量",
    "亮度",
    "熄屏",
    "立即同步",
    "待同步",
    "同步中",
    "同步失败",
    "已同步",
};

const char * const * ui_manifest_projection_static_labels(int *count)
{
    if (count) {
        *count = (int)(sizeof(s_labels) / sizeof(s_labels[0]));
    }
    return s_labels;
}
