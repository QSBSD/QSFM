#include "fs/selection.h"
#include "app.h"
#include "util/path.h"
#include <stdlib.h>
#include <string.h>

char **paths_selected(int *cnt)
{
    int c = 0;
    for (size_t i = 0; i < g.n; i++)
        if (g.v[i].sel)
            c++;
    *cnt = 0;
    if (!c)
        return NULL;
    char **p = calloc((size_t)c, sizeof *p);
    if (!p)
        return NULL;
    int k = 0;
    for (size_t i = 0; i < g.n; i++) {
        if (!g.v[i].sel)
            continue;
        char full[PATH_MAX];
        if (path_join(full, sizeof full, g.cwd, g.v[i].name))
            continue;
        p[k] = strdup(full);
        if (p[k])
            k++;
    }
    *cnt = k;
    return p;
}

void paths_free(char **p, int n)
{
    for (int i = 0; i < n; i++)
        free(p[i]);
    free(p);
}

int sel_count(void)
{
    int c = 0;
    for (size_t i = 0; i < g.n; i++)
        c += g.v[i].sel;
    return c;
}

void clear_sel(void)
{
    for (size_t i = 0; i < g.n; i++)
        g.v[i].sel = false;
}

bool selected_is_dir(void)
{
    const Entry *one = NULL;
    for (size_t i = 0; i < g.n; i++) {
        if (!g.v[i].sel)
            continue;
        if (one)
            return false;
        one = &g.v[i];
    }
    return one && one->is_dir;
}
