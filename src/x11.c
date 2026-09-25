#include "x11.h"
#include "app.h"
#include "config.h"
#include "ui/font.h"
#include "ui/theme.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb_cursor.h>

static xcb_atom_t atom(const char *name)
{
    xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(g.c, xcb_intern_atom(g.c, 0, (uint16_t)strlen(name), name), NULL);
    xcb_atom_t a = r ? r->atom : XCB_ATOM_NONE;
    free(r);
    return a;
}

int x11_init(void)
{
    int sn = 0;
    g.c = xcb_connect(NULL, &sn);
    if (xcb_connection_has_error(g.c)) {
        fprintf(stderr, "QSFM: cannot connect to X server\n");
        return 1;
    }
    fcntl(xcb_get_file_descriptor(g.c), F_SETFD, FD_CLOEXEC);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(g.c));
    for (int i = 0; i < sn; i++)
        xcb_screen_next(&it);
    g.scr = it.data;
    g.depth = g.scr->root_depth;

    g.a_protocols = atom("WM_PROTOCOLS");
    g.a_delete = atom("WM_DELETE_WINDOW");
    g.a_clipboard = atom("CLIPBOARD");
    g.a_targets = atom("TARGETS");
    g.a_urilist = atom("text/uri-list");
    g.a_utf8 = atom("UTF8_STRING");
    g.a_wake = atom("QSFM_WAKE");
    g.a_netwmname = atom("_NET_WM_NAME");

    if (!font_init()) {
        fprintf(stderr, "QSFM: no usable font: %s\n", config_warnings());
        return 1;
    }

    xcb_cursor_context_t *cc = NULL;
    if (xcb_cursor_context_new(g.c, g.scr, &cc) == 0) {
        g.cursor = xcb_cursor_load_cursor(cc, "left_ptr");
        xcb_cursor_context_free(cc);
    }

    metrics_init();
    g.W = sc(cfg.window_width);
    g.H = sc(cfg.window_height);
    if (g.W > g.scr->width_in_pixels)
        g.W = g.scr->width_in_pixels;
    if (g.H > g.scr->height_in_pixels)
        g.H = g.scr->height_in_pixels;
    g.win = xcb_generate_id(g.c);
    uint32_t wv[3] = {XCB_BACK_PIXMAP_NONE,
                      XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS |
                          XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_1_MOTION |
                          XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                      g.cursor};
    uint32_t wm = XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK | (g.cursor ? XCB_CW_CURSOR : 0);
    xcb_create_window(g.c, XCB_COPY_FROM_PARENT, g.win, g.scr->root, 0, 0, (uint16_t)g.W, (uint16_t)g.H, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, g.scr->root_visual, wm, wv);
    const char *title = "QSFM";
    xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, g.win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, 4, "QSFM");
    xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, g.win, g.a_netwmname, g.a_utf8, 8, (uint32_t)strlen(title), title);
    xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, g.win, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, 10, "QSFM\0QSFM");
    xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, g.win, g.a_protocols, XCB_ATOM_ATOM, 32, 1, &g.a_delete);

    uint32_t gv[3] = {cfg.c_text, cfg.c_bg, 0};
    g.gc = xcb_generate_id(g.c);
    xcb_create_gc(g.c, g.gc, g.win,
                  XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES, gv);
    g.back = xcb_generate_id(g.c);
    xcb_create_pixmap(g.c, g.depth, g.back, g.win, (uint16_t)g.W, (uint16_t)g.H);
    g.syms = xcb_key_symbols_alloc(g.c);
    return 0;
}
