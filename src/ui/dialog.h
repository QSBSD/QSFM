#ifndef FM_DIALOG_H
#define FM_DIALOG_H

#include "app.h"
#include "util/textinput.h"

enum { DK_MSG, DK_CONFIRM, DK_INPUT, DK_PROPS };

enum { DA_NONE, DA_RENAME, DA_NEWDIR, DA_NEWFILE, DA_DELETE };

typedef struct {
    bool open, copied;
    int kind, action, w, h, nl;
    xcb_window_t win;
    char title[64];
    char lines[14][256];
    TI in;
    char arg[PATH_MAX];
    char **paths;
    int npaths;
} Dlg;

extern Dlg dlg;

void dlg_open(int kind, int action, const char *title, const char *msg, const char *init, const char *arg);
void dlg_draw(void);
void dlg_key(uint32_t ks, uint32_t cp);
void dlg_press(xcb_button_press_event_t *e);
#endif
