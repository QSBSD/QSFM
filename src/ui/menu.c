#include "ui/menu.h"
#include "fs/mount.h"
#include "fs/selection.h"
#include "ui/actions.h"
#include "ui/clipboard.h"
#include "ui/dialog.h"
#include "ui/gfx.h"
#include "ui/theme.h"
#include <stdio.h>
#include <string.h>

Menu menu;

static void menu_add(const char *label, int id, bool en)
{
    snprintf(menu.it[menu.n].label, sizeof menu.it[0].label, "%s", label);
    menu.it[menu.n].id = id;
    menu.it[menu.n].en = en;
    menu.n++;
}

void menu_draw(void)
{
    xcb_drawable_t save = g.dr;
    g.dr = menu.win;
    gfx_fill(0, 0, menu.w, menu.h, C_MENU);
    gfx_frame(0, 0, menu.w, menu.h, C_BORDER);
    for (int i = 0; i < menu.n; i++) {
        int y = MENU_PAD + i * MENU_IH;
        bool hot = i == menu.hot && menu.it[i].en;
        uint32_t bg = hot ? C_SEL : C_MENU;
        gfx_fill(1, y, menu.w - 2, MENU_IH, bg);
        gfx_text(sc(12), y + (MENU_IH - g.fh) / 2, menu.it[i].label, hot ? C_SEL_TEXT : (menu.it[i].en ? C_TEXT : C_DIM), bg,
             menu.w - sc(24));
    }
    g.dr = save;
}

void menu_close(void)
{
    if (!menu.open)
        return;
    xcb_ungrab_pointer(g.c, XCB_CURRENT_TIME);
    xcb_destroy_window(g.c, menu.win);
    menu.open = false;
    g.dirty = true;
}

static void menu_show(int rx, int ry)
{
    menu.w = MENU_W;
    menu.h = menu.n * MENU_IH + 2 * MENU_PAD;
    menu.hot = -1;
    if (rx + menu.w > g.scr->width_in_pixels)
        rx = g.scr->width_in_pixels - menu.w;
    if (ry + menu.h > g.scr->height_in_pixels)
        ry = g.scr->height_in_pixels - menu.h;
    if (rx < 0)
        rx = 0;
    if (ry < 0)
        ry = 0;
    menu.win = gfx_create_win(rx, ry, menu.w, menu.h, true,
                          XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_POINTER_MOTION);
    xcb_map_window(g.c, menu.win);
    xcb_grab_pointer_cookie_t ck = xcb_grab_pointer(
        g.c, 1, menu.win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
    xcb_discard_reply(g.c, ck.sequence);
    menu.open = true;
}

void menu_list(int rx, int ry, bool on_item)
{
    int ns = sel_count();
    menu.n = 0;
    menu.place = -1;
    if (on_item) {
        menu_add("Открыть", M_OPEN, ns >= 1);
        if (ns == 1 && selected_is_dir())
            menu_add("Открыть в терминале", M_TERM, true);
        menu_add("Копировать", M_COPY, ns >= 1);
        menu_add("Вырезать", M_CUT, ns >= 1);
        menu_add("Переименовать", M_RENAME, ns == 1);
        menu_add("Удалить", M_DELETE, ns >= 1);
        menu_add("Свойства", M_PROPS, true);
    } else {
        menu_add("Новая папка", M_NEWDIR, true);
        menu_add("Новый файл", M_NEWFILE, true);
        menu_add("Вставить", M_PASTE, g.nclip > 0);
    }
    menu_show(rx, ry);
}

void menu_place(int i, int rx, int ry)
{
    if (g.places[i].kind != 2)
        return;
    menu.n = 0;
    menu.place = i;
    if (g.places[i].mounted)
        menu_add("Отмонтировать", M_UMOUNT, strcmp(g.places[i].path, "/") != 0);
    else
        menu_add("Смонтировать", M_MOUNT, true);
    menu_show(rx, ry);
}

static void menu_activate(int id, int place)
{
    menu_close();
    switch (id) {
    case M_OPEN: open_selected(); break;
    case M_COPY: clip_copy(); break;
    case M_CUT: clip_cut(); break;
    case M_TERM: open_terminal_selected(); break;
    case M_PASTE: clip_paste(); break;
    case M_RENAME: ask_rename(); break;
    case M_DELETE: ask_delete(); break;
    case M_NEWDIR: dlg_open(DK_INPUT, DA_NEWDIR, "Новая папка", "Имя папки:", "Новая папка", NULL); break;
    case M_NEWFILE: dlg_open(DK_INPUT, DA_NEWFILE, "Новый файл", "Имя файла:", "Новый файл", NULL); break;
    case M_PROPS: ask_props(); break;
    case M_MOUNT:
        if (place >= 0) start_mount(g.places[place].path, 'm');
        break;
    case M_UMOUNT:
        if (place >= 0) start_mount(g.places[place].path, 'u');
        break;
    }
}

void menu_press(xcb_button_press_event_t *e)
{
    int i = (e->event_y - MENU_PAD) / MENU_IH;
    bool in = e->event_x >= 0 && e->event_x < menu.w && e->event_y >= MENU_PAD && i >= 0 && i < menu.n;
    if (in && e->detail == 1 && menu.it[i].en)
        menu_activate(menu.it[i].id, menu.place);
    else if (!in || e->detail != 1)
        menu_close();
}
