#include "ui/keyboard.h"
#include "app.h"
#include "fs/copy.h"
#include "fs/listing.h"
#include "fs/selection.h"
#include "ui/actions.h"
#include "ui/clipboard.h"
#include "ui/dialog.h"
#include "ui/layout.h"
#include "ui/menu.h"
#include "ui/nav.h"
#include "util/keysym.h"
#include "util/textinput.h"
#include "util/utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void key_syms(xcb_key_press_event_t *e, uint32_t *ks, uint32_t *cp, uint32_t *base)
{
    int shift = (e->state & XCB_MOD_MASK_SHIFT) ? 1 : 0;
    int grp = (e->state >> 13) & 3;
    uint32_t k0 = xcb_key_symbols_get_keysym(g.syms, e->detail, grp * 2);
    uint32_t k1 = xcb_key_symbols_get_keysym(g.syms, e->detail, grp * 2 + 1);
    if (!k0 && grp) {
        k0 = xcb_key_symbols_get_keysym(g.syms, e->detail, 0);
        k1 = xcb_key_symbols_get_keysym(g.syms, e->detail, 1);
    }
    if (!k1)
        k1 = k0;
    if (e->state & XCB_MOD_MASK_LOCK) {
        uint32_t c0 = ks_to_cp(k0), c1 = ks_to_cp(k1);
        if (cp_fold(c0) != c0 || cp_fold(c1) != c1)
            shift ^= 1;
    }
    if (k0 >= 0xff80 && k0 <= 0xffbd && (e->state & XCB_MOD_MASK_2))
        shift ^= 1;
    *ks = shift ? k1 : k0;
    *cp = ks_to_cp(*ks);
    *base = xcb_key_symbols_get_keysym(g.syms, e->detail, 0);
}

static void select_only(int idx)
{
    clear_sel();
    if (idx >= 0 && idx < (int)g.n) {
        g.v[idx].sel = true;
        g.anchor = idx;
        Layout l = layout_get();
        int row = idx / l.cols;
        if (row < g.top)
            g.top = row;
        else if (row >= g.top + l.vis)
            g.top = row - l.vis + 1;
        layout_clamp_top(&l);
    }
}

static void list_key(uint32_t ks)
{
    Layout l = layout_get();
    int cur = g.anchor;
    for (size_t i = 0; i < g.n && cur < 0; i++)
        if (g.v[i].sel)
            cur = (int)i;
    int n = (int)g.n;
    switch (ks) {
    case 0xffff: ask_delete(); break;
    case 0xffbf: ask_rename(); break;
    case 0xffc2: reload_dir(); break;
    case 0xff08: nav_up(); break;
    case 0xff0d: open_selected(); break;
    case 0xff1b: clear_sel(); break;
    case 0xff51: if (n) select_only(cur < 0 ? 0 : (cur > 0 ? cur - 1 : 0)); break;
    case 0xff53: if (n) select_only(cur < 0 ? 0 : (cur < n - 1 ? cur + 1 : n - 1)); break;
    case 0xff52: if (n) select_only(cur < 0 ? n - 1 : (cur >= l.cols ? cur - l.cols : cur)); break;
    case 0xff54:
        if (n)
            select_only(cur < 0 ? 0 : (cur + l.cols < n ? cur + l.cols : (cur / l.cols < (n - 1) / l.cols ? n - 1 : cur)));
        break;
    case 0xff55: if (n) select_only(cur - l.vis * l.cols < 0 ? 0 : cur - l.vis * l.cols); break;
    case 0xff56: if (n) select_only(cur + l.vis * l.cols >= n ? n - 1 : cur + l.vis * l.cols); break;
    case 0xff50: if (n) select_only(0); break;
    case 0xff57: if (n) select_only(n - 1); break;
    }
}

void kb_on_key(xcb_key_press_event_t *e)
{
    uint32_t ks, cp, base;
    key_syms(e, &ks, &cp, &base);
    bool ctrl = (e->state & XCB_MOD_MASK_CONTROL) != 0;
    if (ks == 0xff8d)
        ks = 0xff0d;
    if (ctrl)
        cp = 0;
    if (dlg.open) {
        dlg_key(ks, cp);
        return;
    }
    if (menu.open) {
        if (ks == 0xff1b)
            menu_close();
        return;
    }
    g.dirty = true;
    if (ctrl) {
        switch (base) {
        case 'l':
            g.focus = F_PATH;
            g.path.cur = g.path.len;
            return;
        case 'f':
            g.focus = F_SEARCH;
            g.search.cur = g.search.len;
            return;
        case 'h':
            g.show_hidden = !g.show_hidden;
            reload_dir();
            return;
        case 'c':
            if (g.focus == F_LIST) clip_copy();
            return;
        case 'x':
            if (g.focus == F_LIST) clip_cut();
            return;
        case 'v':
            if (g.focus == F_LIST) clip_paste();
            return;
        case 'a':
            if (g.focus == F_LIST)
                for (size_t i = 0; i < g.n; i++)
                    g.v[i].sel = true;
            return;
        }
        if (g.focus == F_LIST)
            return;
    }
    if (g.focus == F_PATH) {
        if (ks == 0xff0d) {
            char p[PATH_MAX];
            const char *b = g.path.buf, *home = getenv("HOME");
            if (b[0] == '~' && home)
                snprintf(p, sizeof p, "%s%s", home, b + 1);
            else
                snprintf(p, sizeof p, "%s", b);
            g.focus = F_LIST;
            navigate(p);
            ti_set(&g.path, g.cwd);
        } else if (ks == 0xff1b) {
            ti_set(&g.path, g.cwd);
            g.focus = F_LIST;
        } else {
            ti_key(&g.path, ks, cp);
        }
    } else if (g.focus == F_SEARCH) {
        if (ks == 0xff0d) {
            if (g.search.len == 0)
                reload_dir();
            else
                start_load(g.cwd, g.search.buf);
        } else if (ks == 0xff1b) {
            ti_set(&g.search, "");
            g.focus = F_LIST;
            if (g.searching)
                reload_dir();
        } else {
            ti_key(&g.search, ks, cp);
        }
    } else {
        list_key(ks);
    }
}
