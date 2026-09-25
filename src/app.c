#include "app.h"
#include <stdio.h>
#include <string.h>

App g;

void wake(void)
{
    xcb_client_message_event_t ev;
    memset(&ev, 0, sizeof ev);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = g.win;
    ev.type = g.a_wake;
    xcb_send_event(g.c, 0, g.win, XCB_EVENT_MASK_NO_EVENT, (const char *)&ev);
    xcb_flush(g.c);
}

/* the caller holds g.mtx */
void err_msg(const char *m)
{
    if (!g.has_err) {
        snprintf(g.err, sizeof g.err, "%s", m);
        g.has_err = true;
    }
}

void err_set(const char *ctx, int e)
{
    char m[512];
    snprintf(m, sizeof m, "%s: %s", ctx, strerror(e));
    err_msg(m);
}

void post_err(const char *ctx, int e)
{
    pthread_mutex_lock(&g.mtx);
    err_set(ctx, e);
    pthread_mutex_unlock(&g.mtx);
    wake();
}

void post_msg(const char *m)
{
    pthread_mutex_lock(&g.mtx);
    err_msg(m);
    pthread_mutex_unlock(&g.mtx);
    wake();
}
