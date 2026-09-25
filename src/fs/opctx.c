#include "fs/opctx.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>

void ctx_tick(Ctx *c)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long long ms = ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
    if (ms - c->last >= 80) {
        c->last = ms;
        wake();
    }
}

void ctx_set(Ctx *c, const char *p, int e)
{
    snprintf(c->ctx, sizeof c->ctx, "%s", p);
    errno = e;
}
