#ifndef FM_APP_H
#define FM_APP_H

#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "util/textinput.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define MAX_PLACES 48

enum { F_LIST, F_PATH, F_SEARCH };
enum { OP_NONE, OP_COPY, OP_DELETE, OP_MOUNT };

typedef struct {
    char *name;
    off_t size;
    time_t mtime;
    mode_t mode;
    bool is_dir, sel;
} Entry;

typedef struct {
    char label[64];
    char path[512];
    int kind; /* 0 home, 1 desktop, 2 mount */
    bool mounted;
} Place;

typedef struct Job {
    atomic_bool cancel;
    bool hidden;
    char path[PATH_MAX];
    char pat[256];
} Job;

typedef struct {
    xcb_connection_t *c;
    xcb_screen_t *scr;
    xcb_window_t win;
    xcb_pixmap_t back;
    xcb_gcontext_t gc;
    xcb_drawable_t dr;
    xcb_cursor_t cursor;
    xcb_key_symbols_t *syms;
    uint8_t depth;
    int W, H, cw, fh, asc;
    xcb_atom_t a_protocols, a_delete, a_clipboard, a_targets, a_urilist, a_utf8, a_wake, a_netwmname;

    pthread_mutex_t mtx;
    Entry *v;
    size_t n, cap;
    bool loading, searching;
    Job *job;
    char cwd[PATH_MAX];
    long long space_free, space_total; /* of the file system holding cwd; total == 0: unknown */
    int top, anchor, focus;
    TI path, search;
    bool show_hidden;
    Place places[MAX_PLACES];
    int nplaces;

    char *clip_text; /* plain text put on the clipboard (path from the properties dialog) */
    bool clip_is_text;
    char **clip;
    int nclip;
    bool clip_cut;

    atomic_int busy, opkind, reload, places_dirty;
    atomic_llong done, total;
    char nav_path[512];
    char err[512];
    bool has_err;

    bool quit, dirty, drag_sb;
    int drag_off;

    /* rubber-band selection: pending = button 1 pressed in the list, on = the pointer moved far enough */
    bool band_pend, band_on, band_add;
    int band_ax, band_ay, band_cx, band_cy; /* anchor and pointer in list content coordinates */
    int band_px, band_py, band_idx;         /* press position (window) and the entry under it */
    unsigned band_state;
    bool *band_base;                        /* selection before the drag (add mode) */
    int band_n0;
} App;

extern App g;

void wake(void);
void err_msg(const char *m);
void err_set(const char *ctx, int e);
void post_err(const char *ctx, int e);
void post_msg(const char *m);
#endif
