#ifndef FM_THEME_H
#define FM_THEME_H

#include "config.h"

typedef struct {
    int pad, line, fld_h, tb_h, btn, up_x, path_x, search_w;
    int side_w, cell_w, cell_h, st_h, place_h, place_y0, scrl_w;
    int menu_w, menu_ih, menu_pad;
    int icon, icon_up, icon_big;
} Metrics;

extern Metrics met;

int sc(int v);
void metrics_init(void);

#define TB_PAD met.pad
#define FLD_H met.fld_h
#define TB_H met.tb_h
#define BTN met.btn
#define BTN_Y ((met.tb_h - met.btn) / 2)
#define UP_X met.up_x
#define PATH_X met.path_x
#define SEARCH_W met.search_w
#define SIDE_W met.side_w
#define CELL_W met.cell_w
#define CELL_H met.cell_h
#define NAME_LINES 2
#define ST_H met.st_h
#define PLACE_H met.place_h
#define PLACE_Y0 met.place_y0
#define SCRL_W met.scrl_w
#define MENU_W met.menu_w
#define MENU_IH met.menu_ih
#define MENU_PAD met.menu_pad

#define C_BG cfg.c_bg
#define C_TB cfg.c_toolbar
#define C_SIDE cfg.c_sidebar
#define C_BORDER cfg.c_border
#define C_TEXT cfg.c_text
#define C_DIM cfg.c_text_dim
#define C_SEL cfg.c_selection
#define C_SEL_TEXT cfg.c_selection_text
#define C_PLACE_SEL cfg.c_place_selection
#define C_FOCUS cfg.c_focus
#define C_BTN cfg.c_button
#define C_MENU cfg.c_menu
#define C_SCRL cfg.c_scrollbar
#define C_THUMB cfg.c_scrollbar_thumb
#endif
