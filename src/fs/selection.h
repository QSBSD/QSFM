#ifndef FM_SELECTION_H
#define FM_SELECTION_H

#include <stdbool.h>

char **paths_selected(int *cnt);
void paths_free(char **p, int n);
int sel_count(void);
bool selected_is_dir(void); /* exactly one entry selected and it is a folder */
void clear_sel(void);
#endif
