#ifndef FM_OPCTX_H
#define FM_OPCTX_H

#include <limits.h>
#include "app.h"

typedef struct {
    char *buf;
    char ctx[PATH_MAX];
    long long last;
} Ctx;

void ctx_tick(Ctx *c);
void ctx_set(Ctx *c, const char *p, int e);
#endif
