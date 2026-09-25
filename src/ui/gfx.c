#include "ui/gfx.h"
#include "app.h"
#include "ui/font.h"
#include "util/utf8.h"
#include <xcb/xcb_image.h>

void gfx_setfg(uint32_t c)
{
    xcb_change_gc(g.c, g.gc, XCB_GC_FOREGROUND, &c);
}

void gfx_fill(int x, int y, int w, int h, uint32_t col)
{
    if (w <= 0 || h <= 0)
        return;
    xcb_rectangle_t r = {(int16_t)x, (int16_t)y, (uint16_t)w, (uint16_t)h};
    gfx_setfg(col);
    xcb_poly_fill_rectangle(g.c, g.dr, g.gc, 1, &r);
}

void gfx_frame(int x, int y, int w, int h, uint32_t col)
{
    xcb_rectangle_t r = {(int16_t)x, (int16_t)y, (uint16_t)(w - 1), (uint16_t)(h - 1)};
    gfx_setfg(col);
    xcb_poly_rectangle(g.c, g.dr, g.gc, 1, &r);
}

int gfx_u8_to_c2b(const char *s, xcb_char2b_t *out, int max)
{
    int n = 0, k;
    uint32_t cp;
    while (n < max && (k = u8_decode(s, &cp)) > 0) {
        if (cp > 0xFFFF || cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
            cp = '?';
        out[n].byte1 = (uint8_t)(cp >> 8);
        out[n].byte2 = (uint8_t)(cp & 0xFF);
        n++;
        s += k;
    }
    return n;
}

void gfx_draw_c2b(int x, int y, const xcb_char2b_t *s, int n, uint32_t fg, uint32_t bg)
{
    if (n > 255)
        n = 255;
    font_draw(x, y, s, n, fg, bg);
}

/* y is the top of the text box */
void gfx_text(int x, int y, const char *s, uint32_t fg, uint32_t bg, int maxw)
{
    xcb_char2b_t buf[256];
    int n = gfx_u8_to_c2b(s, buf, 255);
    int maxc = maxw / g.cw;
    if (n > maxc) {
        n = maxc;
        if (n >= 3) {
            buf[n - 1].byte1 = 0;
            buf[n - 1].byte2 = '.';
            buf[n - 2] = buf[n - 1];
        }
    }
    gfx_draw_c2b(x, y, buf, n, fg, bg);
}

void gfx_text_right(int xr, int y, const char *s, uint32_t fg, uint32_t bg, int maxw)
{
    int n = u8_len(s);
    int maxc = maxw / g.cw;
    if (n > maxc)
        n = maxc;
    gfx_text(xr - n * g.cw, y, s, fg, bg, maxw);
}

xcb_window_t gfx_create_win(int x, int y, int w, int h, bool override, uint32_t mask)
{
    uint32_t v[4];
    int i = 0;
    uint32_t m = XCB_CW_BACK_PIXMAP;
    v[i++] = XCB_BACK_PIXMAP_NONE;
    if (override) {
        m |= XCB_CW_OVERRIDE_REDIRECT;
        v[i++] = 1;
    }
    m |= XCB_CW_EVENT_MASK;
    v[i++] = mask;
    if (g.cursor) {
        m |= XCB_CW_CURSOR;
        v[i++] = g.cursor;
    }
    xcb_window_t id = xcb_generate_id(g.c);
    xcb_create_window(g.c, XCB_COPY_FROM_PARENT, id, g.scr->root, (int16_t)x, (int16_t)y, (uint16_t)w, (uint16_t)h, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, g.scr->root_visual, m, v);
    return id;
}
