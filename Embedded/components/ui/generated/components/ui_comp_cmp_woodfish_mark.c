// SquareLine Studio 1.6.1 / ewf-device — cmp_woodfish_mark
// Story 3.2：220×220 热区（≥96）；点击由 bindings 接线，generated 不写业务。
#include "../ui.h"

lv_obj_t * ui_woodfish_mark_create(lv_obj_t * comp_parent)
{
    lv_obj_t * root = lv_obj_create(comp_parent);
    lv_obj_set_size(root, 220, 220);
    lv_obj_set_style_radius(root, 110, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x3D2B1F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label = lv_label_create(root);
    lv_label_set_text(label, "木鱼");
    lv_obj_set_style_text_font(label, &ui_font_serif700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);

    ui_comp_woodfish_mark_create_hook(root);
    return root;
}
