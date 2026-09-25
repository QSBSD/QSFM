#ifndef FM_UI_H
#define FM_UI_H

#include <xcb/xcb.h>

void ui_init(void);
void ui_event(xcb_generic_event_t *ev);
void ui_after(void);
#endif
