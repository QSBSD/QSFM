#ifndef FM_GFX_H
#define FM_GFX_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

void gfx_setfg(uint32_t c);
void gfx_fill(int x, int y, int w, int h, uint32_t col);
void gfx_frame(int x, int y, int w, int h, uint32_t col);
int gfx_u8_to_c2b(const char *s, xcb_char2b_t *out, int max);
void gfx_draw_c2b(int x, int y, const xcb_char2b_t *s, int n, uint32_t fg, uint32_t bg);
void gfx_text(int x, int y, const char *s, uint32_t fg, uint32_t bg, int maxw);
void gfx_text_right(int xr, int y, const char *s, uint32_t fg, uint32_t bg, int maxw);
xcb_window_t gfx_create_win(int x, int y, int w, int h, bool override, uint32_t mask);
#endif
