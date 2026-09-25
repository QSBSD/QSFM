#include "ui/dialog.h"
#include "fs/delete.h"
#include "fs/selection.h"
#include "ui/clipboard.h"
#include "ui/gfx.h"
#include "ui/nav.h"
#include "ui/theme.h"
#include "ui/ti_view.h"
#include "util/path.h"
#include "util/utf8.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

Dlg dlg;

static int imax(int a, int b)
{
    return a > b ? a : b;
}

static int dlg_lh(void)
{
    return imax(sc(18), g.fh + sc(5));
}

static int dlg_th(void)
{
    return imax(sc(23), g.fh + sc(8));
}

static int dlg_text_y(void)
{
    return dlg_th() + sc(13);
}

static void dlg_labels(const char *lb[2])
{
    lb[0] = "OK";
    lb[1] = "Отмена";
    if (dlg.kind == DK_CONFIRM) {
        lb[0] = "Да";
        lb[1] = "Нет";
    } else if (dlg.kind == DK_PROPS) {
        lb[0] = dlg.copied ? "Скопировано" : "Копировать путь";
        lb[1] = "OK";
    }
}

static int dlg_btn_w(void)
{
    const char *lb[2];
    dlg_labels(lb);
    int n = imax(u8_len("Отмена"), imax(u8_len(lb[0]), u8_len(lb[1])));
    if (dlg.kind == DK_PROPS)
        n = imax(n, u8_len("Копировать путь"));
    return imax(sc(90), n * g.cw + sc(30));
}

static void dlg_wrap(const char *msg)
{
    int maxc = (dlg.w - 2 * sc(16)) / g.cw;
    if (maxc > 58)
        maxc = 58;
    const char *p = msg;
    dlg.nl = 0;
    while (*p && dlg.nl < 14) {
        char *o = dlg.lines[dlg.nl];
        size_t bytes = 0;
        int cnt = 0;
        while (*p && *p != '\n' && cnt < maxc) {
            uint32_t cp;
            int k = u8_decode(p, &cp);
            memcpy(o + bytes, p, (size_t)k);
            bytes += (size_t)k;
            p += k;
            cnt++;
        }
        o[bytes] = 0;
        dlg.nl++;
        if (*p == '\n')
            p++;
    }
}

static int dlg_input_y(void)
{
    return dlg_text_y() + dlg.nl * dlg_lh() + sc(6);
}

static void dlg_btn(int i, int *x, int *y, int *w, int *h)
{
    int nb = dlg.kind == DK_MSG ? 1 : 2;
    *w = dlg_btn_w();
    *h = FLD_H;
    *y = dlg.h - FLD_H - sc(12);
    *x = nb == 1 ? (dlg.w - *w) / 2 : (i == 0 ? dlg.w / 2 - *w - sc(5) : dlg.w / 2 + sc(5));
}

void dlg_draw(void)
{
    xcb_drawable_t save = g.dr;
    g.dr = dlg.win;
    int m = sc(16);
    gfx_fill(0, 0, dlg.w, dlg.h, C_SIDE);
    gfx_frame(0, 0, dlg.w, dlg.h, C_BORDER);
    gfx_fill(1, 1, dlg.w - 2, dlg_th(), C_SEL);
    gfx_text(sc(10), 1 + (dlg_th() - g.fh) / 2, dlg.title, C_SEL_TEXT, C_SEL, dlg.w - 2 * sc(10));
    for (int i = 0; i < dlg.nl; i++)
        gfx_text(m, dlg_text_y() + i * dlg_lh(), dlg.lines[i], C_TEXT, C_SIDE, dlg.w - 2 * m);
    if (dlg.kind == DK_INPUT)
        ti_draw(&dlg.in, m, dlg_input_y(), dlg.w - 2 * m, FLD_H, true, NULL);
    const char *lb[2];
    dlg_labels(lb);
    for (int i = 0; i < (dlg.kind == DK_MSG ? 1 : 2); i++) {
        int x, y, w, h;
        dlg_btn(i, &x, &y, &w, &h);
        gfx_fill(x, y, w, h, C_BTN);
        gfx_frame(x, y, w, h, C_BORDER);
        int tw = u8_len(lb[i]) * g.cw;
        gfx_text(x + (w - tw) / 2, y + (h - g.fh) / 2, lb[i], C_TEXT, C_BTN, w);
    }
    g.dr = save;
}

void dlg_open(int kind, int action, const char *title, const char *msg, const char *init, const char *arg)
{
    memset(&dlg, 0, sizeof dlg);
    dlg.kind = kind;
    dlg.action = action;
    dlg.w = imax(sc(440), 58 * g.cw + 2 * sc(16));
    if (dlg.w > g.scr->width_in_pixels)
        dlg.w = g.scr->width_in_pixels;
    snprintf(dlg.title, sizeof dlg.title, "%s", title);
    if (arg)
        snprintf(dlg.arg, sizeof dlg.arg, "%s", arg);
    dlg_wrap(msg);
    dlg.h = dlg_text_y() + dlg.nl * dlg_lh() + (kind == DK_INPUT ? FLD_H + sc(12) : sc(6)) + FLD_H + sc(24);
    if (kind == DK_INPUT)
        ti_set(&dlg.in, init ? init : "");
    int x = (g.W - dlg.w) / 2, y = (g.H - dlg.h) / 3;
    xcb_translate_coordinates_reply_t *r =
        xcb_translate_coordinates_reply(g.c, xcb_translate_coordinates(g.c, g.win, g.scr->root, 0, 0), NULL);
    if (r) {
        x += r->dst_x;
        y += r->dst_y;
        free(r);
    }
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    dlg.win = gfx_create_win(x, y, dlg.w, dlg.h, true, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS);
    xcb_map_window(g.c, dlg.win);
    dlg.open = true;
}

static void dlg_close(void)
{
    if (!dlg.open)
        return;
    xcb_destroy_window(g.c, dlg.win);
    if (dlg.paths)
        paths_free(dlg.paths, dlg.npaths);
    dlg.paths = NULL;
    dlg.open = false;
    g.dirty = true;
}

static void dlg_ok(void)
{
    int action = dlg.action;
    char name[1024], arg[PATH_MAX];
    snprintf(name, sizeof name, "%s", dlg.in.buf);
    snprintf(arg, sizeof arg, "%s", dlg.arg);
    char **paths = dlg.paths;
    int np = dlg.npaths;
    dlg.paths = NULL;
    dlg.npaths = 0;
    dlg_close();
    if (action == DA_DELETE) {
        if (paths)
            start_delete(paths, np);
        return;
    }
    if (action != DA_RENAME && action != DA_NEWDIR && action != DA_NEWFILE)
        return;
    if (!name[0] || strchr(name, '/') || !strcmp(name, ".") || !strcmp(name, "..")) {
        err_set(name, EINVAL);
        return;
    }
    char dir[PATH_MAX], to[PATH_MAX];
    struct stat st;
    if (action == DA_RENAME)
        path_parent(arg, dir, sizeof dir);
    else
        snprintf(dir, sizeof dir, "%s", g.cwd);
    if (path_join(to, sizeof to, dir, name)) {
        err_set(name, ENAMETOOLONG);
        return;
    }
    if (action == DA_RENAME) {
        if (strcmp(arg, to) == 0)
            return;
        if (lstat(to, &st) == 0) {
            err_set(to, EEXIST);
            return;
        }
        if (rename(arg, to)) {
            err_set(arg, errno);
            return;
        }
    } else if (action == DA_NEWDIR) {
        if (mkdir(to, 0755)) {
            err_set(to, errno);
            return;
        }
    } else {
        int fd = open(to, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd < 0) {
            err_set(to, errno);
            return;
        }
        close(fd);
    }
    reload_dir();
}

void dlg_key(uint32_t ks, uint32_t cp)
{
    if (ks == 0xff0d) {
        dlg_ok();
        return;
    }
    if (ks == 0xff1b) {
        dlg_close();
        return;
    }
    if (dlg.kind == DK_INPUT && ti_key(&dlg.in, ks, cp))
        dlg_draw();
}

void dlg_press(xcb_button_press_event_t *e)
{
    if (e->detail != 1)
        return;
    for (int i = 0; i < (dlg.kind == DK_MSG ? 1 : 2); i++) {
        int x, y, w, h;
        dlg_btn(i, &x, &y, &w, &h);
        if (e->event_x >= x && e->event_x < x + w && e->event_y >= y && e->event_y < y + h) {
            if (i == 0 && dlg.kind == DK_PROPS) {
                clip_copy_text(dlg.arg);
                dlg.copied = true;
                dlg_draw();
            } else if (i == 0) {
                dlg_ok();
            } else {
                dlg_close();
            }
            return;
        }
    }
    int iy = dlg_input_y();
    if (dlg.kind == DK_INPUT && e->event_y >= iy && e->event_y < iy + FLD_H) {
        ti_click(&dlg.in, sc(16), e->event_x);
        dlg_draw();
    }
}
