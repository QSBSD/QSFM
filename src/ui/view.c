#include "ui/view.h"
#include "app.h"
#include "ui/gfx.h"
#include "ui/icons.h"
#include "ui/layout.h"
#include "ui/nav.h"
#include "ui/theme.h"
#include "ui/ti_view.h"
#include "util/fmt.h"
#include <stdio.h>
#include <string.h>

static void draw_toolbar(void)
{
    gfx_fill(0, 0, g.W, TB_H, C_TB);
    gfx_fill(0, TB_H - 1, g.W, 1, C_BORDER);
    bool en = !nav_at_root();
    uint32_t bbg = en ? C_BTN : C_TB;
    gfx_fill(UP_X, BTN_Y, BTN, BTN, bbg);
    gfx_frame(UP_X, BTN_Y, BTN, BTN, en ? C_BORDER : C_DIM);
    icon_draw(en ? IC_UP : IC_UP_OFF, UP_X + (BTN - met.icon_up) / 2, BTN_Y + (BTN - met.icon_up) / 2, bbg, false);
    ti_draw(&g.path, PATH_X, TB_PAD, layout_path_w(), FLD_H, g.focus == F_PATH, NULL);
    ti_draw(&g.search, layout_search_x(), TB_PAD, SEARCH_W, FLD_H, g.focus == F_SEARCH, "Поиск в папке");
}

static void draw_sidebar(void)
{
    int h = g.H - TB_H - ST_H;
    gfx_fill(0, TB_H, SIDE_W, h, C_SIDE);
    gfx_fill(SIDE_W - 1, TB_H, 1, h, C_BORDER);
    gfx_text(sc(10), TB_H + (PLACE_Y0 - TB_H - g.fh) / 2, "Места", C_TEXT, C_SIDE, SIDE_W - sc(20));
    for (int i = 0; i < g.nplaces; i++) {
        int y = PLACE_Y0 + i * PLACE_H;
        if (y + PLACE_H > g.H - ST_H)
            break;
        Place *p = &g.places[i];
        uint32_t bg = strcmp(g.cwd, p->path) == 0 ? C_PLACE_SEL : C_SIDE;
        int ic = p->kind == 0 ? IC_HOME : p->kind == 1 ? IC_DESK : (p->mounted ? IC_DRIVE : IC_DRIVE_OFF);
        gfx_fill(0, y, SIDE_W - 1, PLACE_H, bg);
        icon_draw(ic, sc(10), y + (PLACE_H - met.icon) / 2, bg, false);
        int tx = sc(10) + met.icon + sc(8);
        gfx_text(tx, y + (PLACE_H - g.fh) / 2, p->label, p->mounted ? C_TEXT : C_DIM, bg, SIDE_W - tx - sc(8));
    }
}

/* name under the icon: wrapped by characters, at most NAME_LINES lines, centred */
static void draw_name(int x, int y, int w, const char *name, uint32_t fg, uint32_t bg)
{
    xcb_char2b_t buf[256];
    int n = gfx_u8_to_c2b(name, buf, 255);
    int maxc = w / g.cw;
    if (maxc < 1)
        maxc = 1;
    if (n > NAME_LINES * maxc) {
        n = NAME_LINES * maxc;
        if (n >= 3) {
            buf[n - 1].byte1 = buf[n - 2].byte1 = 0;
            buf[n - 1].byte2 = buf[n - 2].byte2 = '.';
        }
    }
    for (int i = 0, off = 0; i < NAME_LINES && off < n; i++, off += maxc) {
        int k = n - off < maxc ? n - off : maxc;
        gfx_draw_c2b(x + (w - k * g.cw) / 2, y + i * g.fh, buf + off, k, fg, bg);
    }
}

static void draw_list(void)
{
    Layout l = layout_get();
    layout_clamp_top(&l);
    gfx_fill(SIDE_W, TB_H, g.W - SIDE_W, g.H - TB_H - ST_H, C_BG);
    for (int r = 0; r < l.vis + 1; r++) {
        for (int c = 0; c < l.cols; c++) {
            size_t idx = (size_t)(g.top + r) * (size_t)l.cols + (size_t)c;
            int x = l.x0 + c * l.cellw, y = l.ly + r * l.cellh;
            if (idx >= g.n || y >= l.ly + l.lh)
                break;
            Entry *e = &g.v[idx];
            uint32_t bg = e->sel ? C_SEL : C_BG, fg = e->sel ? C_SEL_TEXT : C_TEXT;
            if (e->sel)
                gfx_fill(x + 1, y + 1, l.cellw - 2, l.cellh - 2, bg);
            icon_draw(e->is_dir ? IC_FOLDER : IC_FILE, x + (l.cellw - met.icon_big) / 2, y + sc(6), bg, e->sel);
            draw_name(x + sc(4), y + sc(6) + met.icon_big + sc(4), l.cellw - 2 * sc(4), e->name, fg, bg);
        }
    }
    if (g.n == 0 && !g.loading)
        gfx_text(l.x0 + sc(8), l.ly + sc(10), g.searching ? "Ничего не найдено" : "Папка пуста", C_DIM, C_BG, sc(300));
    gfx_fill(g.W - SCRL_W, l.ly, SCRL_W, l.lh, C_SCRL);
    if (l.rows > l.vis) {
        int t, h;
        layout_thumb(&l, &t, &h);
        gfx_fill(g.W - SCRL_W + sc(2), t, SCRL_W - 2 * sc(2), h, C_THUMB);
    }
    if (g.band_on) {
        int ax = l.x0 + g.band_ax, ay = l.ly + g.band_ay - g.top * l.cellh;
        int bx = l.x0 + g.band_cx, by = l.ly + g.band_cy - g.top * l.cellh;
        int x0 = ax < bx ? ax : bx, x1 = ax < bx ? bx : ax, y0 = ay < by ? ay : by, y1 = ay < by ? by : ay;
        int lx = SIDE_W, rx = g.W - SCRL_W, ty = l.ly, byy = l.ly + l.lh;
        x0 = x0 < lx ? lx : x0;
        x1 = x1 > rx ? rx : x1;
        y0 = y0 < ty ? ty : y0;
        y1 = y1 > byy ? byy : y1;
        if (x1 - x0 > 1 && y1 - y0 > 1)
            gfx_frame(x0, y0, x1 - x0, y1 - y0, C_DIM);
    }
}

static void draw_status(void)
{
    int y = g.H - ST_H;
    char s[256];
    gfx_fill(0, y, g.W, ST_H, C_TB);
    gfx_fill(0, y, g.W, 1, C_BORDER);
    s[0] = 0;
    if (g.space_total > 0) {
        char fr[32], tot[32];
        human_size(g.space_free, fr, sizeof fr);
        human_size(g.space_total, tot, sizeof tot);
        snprintf(s, sizeof s, "Свободно %s из %s", fr, tot);
    }
    int tw = g.W - sc(8);
    if (atomic_load(&g.busy)) {
        static const char *names[] = {"", "Копирование", "Удаление", "Монтирование"};
        int op = atomic_load(&g.opkind);
        long long tot = atomic_load(&g.total), dn = atomic_load(&g.done);
        char sz[32], sz2[32];
        if (op == OP_COPY && tot > 0) {
            int pw = sc(210), ph = sc(10);
            human_size(dn, sz, sizeof sz);
            human_size(tot, sz2, sizeof sz2);
            snprintf(s, sizeof s, "%s: %lld%% (%s / %s)", names[op], dn * 100 / tot, sz, sz2);
            gfx_fill(g.W - pw - sc(8), y + (ST_H - ph) / 2, pw, ph, C_SCRL);
            gfx_fill(g.W - pw - sc(8), y + (ST_H - ph) / 2, (int)(pw * dn / tot), ph, C_THUMB);
            tw = g.W - pw - sc(20);
        } else {
            snprintf(s, sizeof s, "%s…", names[op >= 0 && op <= 3 ? op : 0]);
        }
    }
    gfx_text(sc(8), y + (ST_H - g.fh) / 2, s, C_TEXT, C_TB, tw);
}

void redraw(void)
{
    g.dr = g.back;
    gfx_fill(0, 0, g.W, g.H, C_BG);
    draw_list();
    draw_sidebar();
    draw_toolbar();
    draw_status();
    xcb_copy_area(g.c, g.back, g.win, g.gc, 0, 0, 0, 0, (uint16_t)g.W, (uint16_t)g.H);
}
