#ifndef FM_CLIPBOARD_H
#define FM_CLIPBOARD_H

#include <xcb/xcb.h>

void clip_copy(void);
void clip_cut(void);
void clip_paste(void);
void clip_copy_text(const char *text);
void clip_on_selreq(xcb_selection_request_event_t *r);
#endif
