#include "ui/mouse.h"
#include "app.h"
#include "config.h"
#include "fs/launch.h"
#include "fs/mount.h"
#include "fs/selection.h"
#include "ui/dialog.h"
#include "ui/layout.h"
#include "ui/menu.h"
#include "ui/nav.h"
#include "ui/theme.h"
#include "ui/ti_view.h"
#include "util/path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int place_at(int by)
{
    if (by < PLACE_Y0)
        return -1;
    int i = (by - PLACE_Y0) / PLACE_H;
    return i < g.nplaces ? i : -1;
}

static void list_click(int idx, int b, unsigned state, int rx, int ry)
{
    if (idx < 0 || idx >= (int)g.n) {
        clear_sel();
        if (b == 3)
            menu_list(rx, ry, false);
        return;
    }
    Entry *en = &g.v[idx];
    if (b == 3) {
        if (!en->sel) {
            clear_sel();
            en->sel = true;
            g.anchor = idx;
        }
        menu_list(rx, ry, true);
        return;
    }
    if (state & XCB_MOD_MASK_CONTROL) {
        en->sel = !en->sel;
        g.anchor = idx;
        return;
    }
    if ((state & XCB_MOD_MASK_SHIFT) && g.anchor >= 0) {
        int a = g.anchor < idx ? g.anchor : idx, z = g.anchor < idx ? idx : g.anchor;
        clear_sel();
        for (int i = a; i <= z && i < (int)g.n; i++)
            g.v[i].sel = true;
        return;
    }
    char full[PATH_MAX];
    if (path_join(full, sizeof full, g.cwd, en->name))
        return;
    if (en->is_dir) {
        navigate(full);
        return;
    }
    clear_sel();
    en->sel = true;
    g.anchor = idx;
    open_file(full);
}

void mouse_on_press(xcb_button_press_event_t *e)
{
    if (e->event != g.win) {
        if (menu.open && e->event == menu.win)
            menu_press(e);
        else if (dlg.open && e->event == dlg.win)
            dlg_press(e);
        return;
    }
    if (dlg.open)
        return;
    if (menu.open) {
        menu_close();
        return;
    }
    int bx = e->event_x, by = e->event_y, b = e->detail;
    Layout l = layout_get();
    if (b == 4 || b == 5) {
        g.top += b == 4 ? -cfg.scroll_lines : cfg.scroll_lines;
        layout_clamp_top(&l);
        g.dirty = true;
        return;
    }
    if (b != 1 && b != 3)
        return;
    g.dirty = true;
    if (by < TB_H) {
        if (b == 1 && bx >= UP_X && bx < UP_X + BTN && by >= BTN_Y && by < BTN_Y + BTN) {
            g.focus = F_LIST;
            nav_up();
        } else if (b == 1 && bx >= PATH_X && bx < PATH_X + layout_path_w() && by >= TB_PAD && by < TB_PAD + FLD_H) {
            g.focus = F_PATH;
            ti_click(&g.path, PATH_X, bx);
        } else if (b == 1 && bx >= layout_search_x() && bx < layout_search_x() + SEARCH_W && by >= TB_PAD && by < TB_PAD + FLD_H) {
            g.focus = F_SEARCH;
            ti_click(&g.search, layout_search_x(), bx);
        } else {
            g.focus = F_LIST;
        }
        return;
    }
    g.focus = F_LIST;
    if (bx < SIDE_W) {
        int i = place_at(by);
        if (i < 0)
            return;
        if (b == 3) {
            menu_place(i, e->root_x, e->root_y);
        } else if (!g.places[i].mounted) {
            start_mount(g.places[i].path, 'm');
        } else {
            navigate(g.places[i].path);
        }
        return;
    }
    if (by < l.ly || by >= l.ly + l.lh)
        return;
    if (bx >= g.W - SCRL_W) {
        if (b != 1)
            return;
        int t, h;
        layout_thumb(&l, &t, &h);
        if (by >= t && by < t + h) {
            g.drag_sb = true;
            g.drag_off = by - t;
        } else {
            g.top += by < t ? -l.vis : l.vis;
            layout_clamp_top(&l);
        }
        return;
    }
    int idx = layout_hit(&l, bx, by);
    if (b == 3) {
        list_click(idx, 3, e->state, e->root_x, e->root_y);
        return;
    }
    /* the click itself is done on release, unless the pointer is dragged: then a rubber band selects */
    g.band_pend = true;
    g.band_on = false;
    g.band_px = bx;
    g.band_py = by;
    g.band_idx = idx;
    g.band_state = e->state;
    g.band_ax = bx - l.x0;
    g.band_ay = g.top * l.cellh + (by - l.ly);
}

static void band_apply(const Layout *l)
{
    int x0 = g.band_ax < g.band_cx ? g.band_ax : g.band_cx, x1 = g.band_ax < g.band_cx ? g.band_cx : g.band_ax;
    int y0 = g.band_ay < g.band_cy ? g.band_ay : g.band_cy, y1 = g.band_ay < g.band_cy ? g.band_cy : g.band_ay;
    int pad = sc(4);
    for (int i = 0; i < (int)g.n; i++) {
        int c = i % l->cols, r = i / l->cols;
        int ix0 = c * l->cellw + pad, ix1 = (c + 1) * l->cellw - pad;
        int iy0 = r * l->cellh + pad, iy1 = (r + 1) * l->cellh - pad;
        bool hit = ix0 < x1 && ix1 > x0 && iy0 < y1 && iy1 > y0;
        g.v[i].sel = hit || (g.band_add && g.band_base && i < g.band_n0 && g.band_base[i]);
    }
}

void mouse_on_release(xcb_button_release_event_t *e)
{
    if (e->detail != 1)
        return;
    g.drag_sb = false;
    if (!g.band_pend)
        return;
    bool moved = g.band_on;
    g.band_pend = g.band_on = false;
    free(g.band_base);
    g.band_base = NULL;
    g.dirty = true;
    if (!moved && e->event == g.win && !dlg.open && !menu.open)
        list_click(g.band_idx, 1, g.band_state, e->root_x, e->root_y);
}

void mouse_on_motion(xcb_motion_notify_event_t *e)
{
    if (menu.open && e->event == menu.win) {
        int i = (e->event_y - MENU_PAD) / MENU_IH;
        if (e->event_y < MENU_PAD || i >= menu.n || e->event_x < 0 || e->event_x >= menu.w)
            i = -1;
        if (i != menu.hot) {
            menu.hot = i;
            menu_draw();
        }
        return;
    }
    if (g.band_pend && e->event == g.win) {
        Layout l = layout_get();
        int dx = e->event_x - g.band_px, dy = e->event_y - g.band_py;
        if (!g.band_on && dx * dx + dy * dy > sc(4) * sc(4)) {
            g.band_on = true;
            g.band_add = (g.band_state & (XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_SHIFT)) != 0;
            g.band_n0 = (int)g.n;
            g.band_base = NULL;
            if (g.band_add && g.n) {
                g.band_base = malloc(g.n * sizeof *g.band_base);
                for (size_t i = 0; g.band_base && i < g.n; i++)
                    g.band_base[i] = g.v[i].sel;
            }
            g.anchor = -1;
        }
        if (g.band_on) {
            if (e->event_y < l.ly)
                g.top--;
            else if (e->event_y >= l.ly + l.lh)
                g.top++;
            layout_clamp_top(&l);
            int cx = e->event_x - l.x0, cy = g.top * l.cellh + (e->event_y - l.ly);
            int mxw = l.cols * l.cellw, mxh = l.rows * l.cellh;
            g.band_cx = cx < 0 ? 0 : cx > mxw ? mxw : cx;
            g.band_cy = cy < 0 ? 0 : cy > mxh ? mxh : cy;
            band_apply(&l);
            g.dirty = true;
        }
        return;
    }
    if (g.drag_sb && e->event == g.win) {
        Layout l = layout_get();
        int t, h;
        layout_thumb(&l, &t, &h);
        int span = l.lh - h, maxtop = l.rows - l.vis;
        if (span > 0 && maxtop > 0) {
            g.top = (e->event_y - g.drag_off - l.ly) * maxtop / span;
            layout_clamp_top(&l);
            g.dirty = true;
        }
    }
}
