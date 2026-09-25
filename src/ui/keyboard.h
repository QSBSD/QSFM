#ifndef FM_KEYBOARD_H
#define FM_KEYBOARD_H

#include <xcb/xcb.h>

void kb_on_key(xcb_key_press_event_t *e);
#endif
