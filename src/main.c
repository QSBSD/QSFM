#include "app.h"
#include "config.h"
#include "x11.h"
#include "fs/places.h"
#include "ui/nav.h"
#include "ui/ui.h"
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    config_load();
    setlocale(LC_TIME, ""); /* month and weekday names in date_format */
    pthread_mutex_init(&g.mtx, NULL);
    if (x11_init())
        return 1;
    g.dr = g.back;
    g.anchor = -1;
    g.focus = F_LIST;
    g.show_hidden = cfg.show_hidden;

    ui_init();
    places_refresh();
    pthread_mutex_lock(&g.mtx);
    const char *home = getenv("HOME");
    char start[PATH_MAX];
    if (cfg.start_dir[0] == '~' && home)
        snprintf(start, sizeof start, "%s%s", home, cfg.start_dir + 1);
    else
        snprintf(start, sizeof start, "%s", cfg.start_dir);
    if (!start[0])
        snprintf(start, sizeof start, "%s", home && *home ? home : "/");
    navigate(start);
    if (!g.cwd[0])
        navigate(home && *home ? home : "/");
    if (!g.cwd[0])
        navigate("/");
    if (config_warnings()[0])
        err_msg(config_warnings());
    pthread_mutex_unlock(&g.mtx);

    xcb_map_window(g.c, g.win);
    xcb_flush(g.c);

    while (!g.quit) {
        xcb_generic_event_t *ev = xcb_wait_for_event(g.c);
        if (!ev)
            break;
        pthread_mutex_lock(&g.mtx);
        do {
            ui_event(ev);
            free(ev);
        } while (!g.quit && (ev = xcb_poll_for_event(g.c)));
        if (!g.quit)
            ui_after();
        pthread_mutex_unlock(&g.mtx);
        xcb_flush(g.c);
    }
    pthread_mutex_lock(&g.mtx);
    if (g.job)
        atomic_store(&g.job->cancel, true);
    pthread_mutex_unlock(&g.mtx);
    xcb_disconnect(g.c);
    return 0;
}
