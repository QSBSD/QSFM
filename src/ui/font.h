#ifndef FM_FONT_H
#define FM_FONT_H

#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

/* loads cfg.font (fontconfig name or path to .ttf/.otf) at font_size * scale; sets g.cw, g.fh, g.asc */
bool font_init(void);
/* draws n UCS-2 characters on a grid of g.cw x g.fh cells; y is the top of the text box */
void font_draw(int x, int y, const xcb_char2b_t *s, int n, uint32_t fg, uint32_t bg);
#endif
