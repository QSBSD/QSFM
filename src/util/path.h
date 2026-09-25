#ifndef FM_PATH_H
#define FM_PATH_H

#include <stddef.h>

int path_join(char *out, size_t n, const char *dir, const char *name);
const char *path_base(const char *p);
void path_parent(const char *p, char *out, size_t n);
#endif
