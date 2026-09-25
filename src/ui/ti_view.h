#ifndef FM_TI_VIEW_H
#define FM_TI_VIEW_H

#include <stdbool.h>
#include "util/textinput.h"

void ti_draw(TI *t, int x, int y, int w, int h, bool focus, const char *hint);
void ti_click(TI *t, int x, int mx);
#endif
