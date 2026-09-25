#include "ui/ui.h"
#include "app.h"
#include "fs/places.h"
#include "ui/clipboard.h"
#include "ui/dialog.h"
#include "ui/icons.h"
#include "ui/keyboard.h"
#include "ui/menu.h"
#include "ui/mouse.h"
#include "ui/nav.h"
#include "ui/view.h"
#include <stdio.h>
#include <string.h>

void ui_init(void)
{
    icons_init();
}

void ui_event(xcb_generic_event_t *ev)
{
    switch (ev->response_type & 0x7f) {
    case XCB_EXPOSE: {
        xcb_expose_event_t *e = (xcb_expose_event_t *)ev;
        if (e->window == g.win)
            g.dirty = true;
        else if (dlg.open && e->window == dlg.win)
            dlg_draw();
        else if (menu.open && e->window == menu.win)
            menu_draw();
        break;
    }
    case XCB_CONFIGURE_NOTIFY: {
        xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *)ev;
        if (e->window == g.win && (e->width != g.W || e->height != g.H)) {
            g.W = e->width;
            g.H = e->height;
            xcb_free_pixmap(g.c, g.back);
            g.back = xcb_generate_id(g.c);
            xcb_create_pixmap(g.c, g.depth, g.back, g.win, (uint16_t)g.W, (uint16_t)g.H);
            g.dirty = true;
        }
        break;
    }
    case XCB_KEY_PRESS:
        kb_on_key((xcb_key_press_event_t *)ev);
        break;
    case XCB_BUTTON_PRESS:
        mouse_on_press((xcb_button_press_event_t *)ev);
        break;
    case XCB_BUTTON_RELEASE:
        mouse_on_release((xcb_button_release_event_t *)ev);
        break;
    case XCB_MOTION_NOTIFY:
        mouse_on_motion((xcb_motion_notify_event_t *)ev);
        break;
    case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t *e = (xcb_client_message_event_t *)ev;
        if (e->type == g.a_protocols && e->data.data32[0] == g.a_delete)
            g.quit = true;
        else if (e->type == g.a_wake)
            g.dirty = true;
        break;
    }
    case XCB_SELECTION_REQUEST:
        clip_on_selreq((xcb_selection_request_event_t *)ev);
        break;
    }
}

/* called after each batch of events, g.mtx held */
void ui_after(void)
{
    if (atomic_exchange(&g.reload, 0))
        reload_dir();
    if (atomic_exchange(&g.places_dirty, 0)) {
        places_refresh();
        g.dirty = true;
    }
    if (g.nav_path[0]) {
        char p[512];
        snprintf(p, sizeof p, "%s", g.nav_path);
        g.nav_path[0] = 0;
        navigate(p);
    }
    if (g.has_err && !dlg.open) {
        char m[512];
        snprintf(m, sizeof m, "%s", g.err);
        g.has_err = false;
        menu_close();
        dlg_open(DK_MSG, DA_NONE, "Ошибка", m, NULL, NULL);
    }
    if (g.dirty) {
        redraw();
        g.dirty = false;
    }
}
