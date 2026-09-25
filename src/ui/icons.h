#ifndef FM_ICONS_H
#define FM_ICONS_H

#include <stdbool.h>
#include <stdint.h>

enum { IC_FOLDER, IC_FILE, IC_UP, IC_UP_OFF, IC_HOME, IC_DESK, IC_DRIVE, IC_DRIVE_OFF, IC_COUNT };

void icons_init(void);
/* on_sel: the icon is drawn on a selected row (uses the selection text colour) */
void icon_draw(int id, int x, int y, uint32_t bg, bool on_sel);
#endif
