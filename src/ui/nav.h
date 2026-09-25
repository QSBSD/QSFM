#ifndef FM_NAV_H
#define FM_NAV_H

#include <stdbool.h>

bool nav_at_root(void);
void reload_dir(void);
void navigate(const char *path);
void nav_up(void);
#endif
