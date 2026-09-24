#ifndef UI_SCR_SHELL_H
#define UI_SCR_SHELL_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
extern void ui_scr_shell_screen_init(void);
extern void ui_scr_shell_screen_destroy(void);
extern lv_obj_t * ui_scr_shell;
extern lv_obj_t * ui_shell_pager;
extern lv_obj_t * ui_page_muyu_root;
extern lv_obj_t * ui_page_jingwen_root;
extern lv_obj_t * ui_page_tongji_root;
extern lv_obj_t * ui_muyu_statusbar;
extern lv_obj_t * ui_jingwen_statusbar;
extern lv_obj_t * ui_tongji_statusbar;
extern lv_obj_t * ui_muyu_woodfish;
extern lv_obj_t * ui_muyu_title;
extern lv_obj_t * ui_jingwen_title;
extern lv_obj_t * ui_tongji_title;
extern lv_obj_t * ui_muyu_glyph_slots[7];
extern lv_obj_t * ui_muyu_scripture_progress;
extern lv_obj_t * ui_muyu_today_hint;
extern lv_obj_t * ui_muyu_tap_rings[3];
extern lv_obj_t * ui_muyu_modal_done;
extern lv_obj_t * ui_muyu_modal_card;
extern lv_obj_t * ui_muyu_modal_title;
extern lv_obj_t * ui_muyu_modal_summary;
extern lv_obj_t * ui_muyu_modal_btn_restart;
extern lv_obj_t * ui_muyu_modal_btn_exit;
extern lv_obj_t * ui_jingwen_history;
extern lv_obj_t * ui_jingwen_history_content;
extern lv_obj_t * ui_jingwen_scripture_progress;
extern lv_obj_t * ui_jingwen_round_index;
extern lv_obj_t * ui_jingwen_glyph_current;
extern lv_obj_t * ui_jingwen_row_boxes[24];
extern lv_obj_t * ui_jingwen_row_slots[24][13];
extern lv_obj_t * ui_tongji_divider;
extern lv_obj_t * ui_tongji_today_card;
extern lv_obj_t * ui_tongji_today_value;
extern lv_obj_t * ui_tongji_total_card;
extern lv_obj_t * ui_tongji_total_value;
extern lv_obj_t * ui_tongji_progress_card;
extern lv_obj_t * ui_tongji_progress_arc;
extern lv_obj_t * ui_tongji_round_label;
extern lv_obj_t * ui_tongji_progress_value;
extern lv_obj_t * ui_tongji_progress_percent;
extern lv_obj_t * ui_tongji_round_index;
extern lv_obj_t * ui_tongji_round_status;
#ifdef __cplusplus
}
#endif
#endif
