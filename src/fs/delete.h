#ifndef FM_DELETE_H
#define FM_DELETE_H

#include "fs/opctx.h"

int rm_tree(const char *path, Ctx *c);
void start_delete(char **paths, int n);
#endif
