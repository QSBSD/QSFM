#include "ui/clipboard.h"
#include "app.h"
#include "fs/copy.h"
#include "fs/selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void clip_take(bool cut)
{
    int n;
    char **p = paths_selected(&n);
    if (!p)
        return;
    if (g.clip)
        paths_free(g.clip, g.nclip);
    g.clip = p;
    g.nclip = n;
    g.clip_cut = cut;
    g.clip_is_text = false;
    xcb_set_selection_owner(g.c, g.win, g.a_clipboard, XCB_CURRENT_TIME);
}

void clip_copy(void)
{
    clip_take(false);
}

void clip_cut(void)
{
    clip_take(true);
}

void clip_copy_text(const char *text)
{
    char *t = strdup(text);
    if (!t)
        return;
    free(g.clip_text);
    g.clip_text = t;
    g.clip_is_text = true;
    xcb_set_selection_owner(g.c, g.win, g.a_clipboard, XCB_CURRENT_TIME);
}

void clip_paste(void)
{
    if (g.nclip == 0)
        return;
    if (g.clip_cut && atomic_load(&g.busy)) {
        err_msg("Операция уже выполняется");
        return;
    }
    char **p = calloc((size_t)g.nclip, sizeof *p);
    if (!p)
        return;
    int k = 0;
    for (int i = 0; i < g.nclip; i++) {
        p[k] = strdup(g.clip[i]);
        if (p[k])
            k++;
    }
    if (g.clip_cut) {
        paths_free(g.clip, g.nclip);
        g.clip = NULL;
        g.nclip = 0;
        g.clip_cut = false;
        start_move(p, k, g.cwd);
    } else {
        start_copy(p, k, g.cwd);
    }
}

static char *clip_build(bool uri, size_t *len)
{
    size_t cap = 16;
    for (int i = 0; i < g.nclip; i++)
        cap += strlen(g.clip[i]) * 3 + 16;
    char *s = malloc(cap);
    size_t n = 0;
    if (!s)
        return NULL;
    for (int i = 0; i < g.nclip; i++) {
        if (uri) {
            n += (size_t)sprintf(s + n, "file://");
            for (const unsigned char *p = (const unsigned char *)g.clip[i]; *p; p++) {
                if ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || strchr("-._~/", *p))
                    s[n++] = (char)*p;
                else
                    n += (size_t)sprintf(s + n, "%%%02X", *p);
            }
            s[n++] = '\r';
            s[n++] = '\n';
        } else {
            n += (size_t)sprintf(s + n, "%s\n", g.clip[i]);
        }
    }
    *len = n;
    return s;
}

void clip_on_selreq(xcb_selection_request_event_t *r)
{
    xcb_selection_notify_event_t n;
    memset(&n, 0, sizeof n);
    n.response_type = XCB_SELECTION_NOTIFY;
    n.requestor = r->requestor;
    n.selection = r->selection;
    n.target = r->target;
    n.time = r->time;
    n.property = XCB_ATOM_NONE;
    xcb_atom_t prop = r->property ? r->property : r->target;
    if (r->selection == g.a_clipboard && g.clip_is_text && g.clip_text) {
        if (r->target == g.a_targets) {
            xcb_atom_t t[3] = {g.a_targets, g.a_utf8, XCB_ATOM_STRING};
            xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, r->requestor, prop, XCB_ATOM_ATOM, 32, 3, t);
            n.property = prop;
        } else if (r->target == g.a_utf8 || r->target == XCB_ATOM_STRING) {
            xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, r->requestor, prop, r->target, 8,
                                (uint32_t)strlen(g.clip_text), g.clip_text);
            n.property = prop;
        }
    } else if (r->selection == g.a_clipboard && g.nclip > 0) {
        if (r->target == g.a_targets) {
            xcb_atom_t t[4] = {g.a_targets, g.a_urilist, g.a_utf8, XCB_ATOM_STRING};
            xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, r->requestor, prop, XCB_ATOM_ATOM, 32, 4, t);
            n.property = prop;
        } else if (r->target == g.a_urilist || r->target == g.a_utf8 || r->target == XCB_ATOM_STRING) {
            size_t len = 0;
            char *s = clip_build(r->target == g.a_urilist, &len);
            if (s) {
                xcb_change_property(g.c, XCB_PROP_MODE_REPLACE, r->requestor, prop, r->target, 8, (uint32_t)len, s);
                free(s);
                n.property = prop;
            }
        }
    }
    xcb_send_event(g.c, 0, r->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&n);
    xcb_flush(g.c);
}
