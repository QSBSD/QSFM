#include "ui/ti_view.h"
#include "app.h"
#include "ui/gfx.h"
#include "ui/theme.h"
#include "util/textinput.h"

void ti_draw(TI *t, int x, int y, int w, int h, bool focus, const char *hint)
{
    xcb_char2b_t all[1024];
    int n = gfx_u8_to_c2b(t->buf, all, 1023);
    int pad = sc(4);
    int vis = (w - 2 * pad) / g.cw;
    int ci = ti_cur_index(t);
    int first = t->first;
    if (ci < first)
        first = ci;
    if (ci > first + vis - 1)
        first = ci - vis + 1;
    if (first < 0)
        first = 0;
    t->first = first;
    int ty = y + (h - g.fh) / 2;
    gfx_fill(x, y, w, h, C_BG);
    gfx_frame(x, y, w, h, focus ? C_FOCUS : C_BORDER);
    if (focus)
        gfx_frame(x + 1, y + 1, w - 2, h - 2, C_FOCUS);
    int cnt = n - first;
    if (cnt > vis)
        cnt = vis;
    if (cnt > 0)
        gfx_draw_c2b(x + pad, ty, all + first, cnt, C_TEXT, C_BG);
    else if (!focus && n == 0 && hint)
        gfx_text(x + pad, ty, hint, C_DIM, C_BG, w - 2 * pad);
    if (focus)
        gfx_fill(x + pad + (ci - first) * g.cw, ty, met.line, g.fh, C_TEXT);
}

void ti_click(TI *t, int x, int mx)
{
    int idx = t->first + (mx - x - sc(4) + g.cw / 2) / g.cw;
    if (idx < 0)
        idx = 0;
    t->cur = ti_index_to_byte(t, idx);
}
