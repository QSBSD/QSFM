#include "util/path.h"
#include <stdio.h>
#include <string.h>

int path_join(char *out, size_t n, const char *dir, const char *name)
{
    int r;
    if (strcmp(dir, "/") == 0)
        r = snprintf(out, n, "/%s", name);
    else
        r = snprintf(out, n, "%s/%s", dir, name);
    return (r < 0 || (size_t)r >= n) ? -1 : 0;
}

const char *path_base(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

void path_parent(const char *p, char *out, size_t n)
{
    snprintf(out, n, "%s", p);
    char *s = strrchr(out, '/');
    if (!s || s == out)
        snprintf(out, n, "/");
    else
        *s = 0;
}
