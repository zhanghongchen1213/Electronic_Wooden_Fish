// SquareLine Studio 1.6.1 / ewf-device — cmp_woodfish_mark
// UI 对齐批次：透明 tap-zone 348×216（≥96 触区）+ 木鱼位图 300×180
// （HTML woodfish tap-zone/anatomy）；点击由 bindings 接线，generated 不写业务。
#include "../ui.h"
#include "ui_muyu_constants.h"

lv_obj_t * ui_woodfish_mark_create(lv_obj_t * comp_parent)
{
    lv_obj_t * root = lv_obj_create(comp_parent);
    lv_obj_set_size(root, EWF_UI_MUYU_TAPZONE_W, EWF_UI_MUYU_TAPZONE_H);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);

    /* woodfish-anatomy：非交互器物位图，轮廓与设计参考一致。 */
    lv_obj_t * img = lv_img_create(root);
    lv_img_set_src(img, &ui_img_woodfish);
    lv_obj_set_pos(img, EWF_UI_MUYU_WOODFISH_DX, EWF_UI_MUYU_WOODFISH_DY);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

    ui_comp_woodfish_mark_create_hook(root);
    return root;
}
