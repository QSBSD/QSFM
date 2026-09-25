#ifndef FM_MENU_H
#define FM_MENU_H

#include "app.h"

enum { M_OPEN = 1, M_COPY, M_PASTE, M_RENAME, M_DELETE, M_NEWDIR, M_NEWFILE, M_PROPS, M_MOUNT, M_UMOUNT, M_CUT, M_TERM };

typedef struct {
    bool open;
    xcb_window_t win;
    int n, w, h, hot, place;
    struct {
        char label[48];
        int id;
        bool en;
    } it[10];
} Menu;

extern Menu menu;

void menu_draw(void);
void menu_close(void);
void menu_list(int rx, int ry, bool on_item);
void menu_place(int i, int rx, int ry);
void menu_press(xcb_button_press_event_t *e);
#endif
