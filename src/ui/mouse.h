#ifndef FM_MOUSE_H
#define FM_MOUSE_H

#include <xcb/xcb.h>

void mouse_on_press(xcb_button_press_event_t *e);
void mouse_on_release(xcb_button_release_event_t *e);
void mouse_on_motion(xcb_motion_notify_event_t *e);
#endif
