/**
 * @file     ui_manifest_projection.h
 * @brief    EWF 壳层静态标签投影（字体合同 runtime 路径）。
 * @author   ZHC
 * @date     2026-09-24
 */
#ifndef EWF_UI_MANIFEST_PROJECTION_H
#define EWF_UI_MANIFEST_PROJECTION_H
#ifdef __cplusplus
extern "C" {
#endif
/** 返回壳层静态产品文案集合，供字体合同扫描。 */
const char * const * ui_manifest_projection_static_labels(int *count);
#ifdef __cplusplus
}
#endif
#endif
