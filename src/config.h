#ifndef QSFM_CONFIG_H
#define QSFM_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double scale;
    char font[256];
    int font_size;
    int window_width, window_height;
    int sidebar_width, icon_size, place_icon_size, up_icon_size, cell_width;
    bool show_hidden;
    int scroll_lines;
    char date_format[64];
    char start_dir[512];
    char opener[128];
    char terminal[256];

    uint32_t c_bg, c_toolbar, c_sidebar, c_border, c_text, c_text_dim;
    uint32_t c_selection, c_selection_text, c_place_selection, c_focus;
    uint32_t c_button, c_menu, c_scrollbar, c_scrollbar_thumb, c_icon, c_icon_dim;
} Config;

extern Config cfg;

void config_load(void);
/* collects problems found at startup; they are shown in a dialog and printed to stderr */
void config_warn(const char *fmt, ...);
const char *config_warnings(void);
#endif
