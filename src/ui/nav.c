#include "ui/nav.h"
#include "app.h"
#include "fs/listing.h"
#include "util/path.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>

bool nav_at_root(void)
{
    return strcmp(g.cwd, "/") == 0;
}

static void update_space(void)
{
    struct statvfs sv;
    if (statvfs(g.cwd, &sv) == 0) {
        g.space_total = (long long)sv.f_blocks * (long long)sv.f_frsize;
        g.space_free = (long long)sv.f_bavail * (long long)sv.f_frsize;
    } else {
        g.space_total = g.space_free = 0;
    }
}

void reload_dir(void)
{
    update_space();
    ti_set(&g.search, "");
    start_load(g.cwd, "");
    g.dirty = true;
}

void navigate(const char *path)
{
    char rp[PATH_MAX];
    struct stat st;
    if (!realpath(path, rp)) {
        err_set(path, errno);
        return;
    }
    if (stat(rp, &st)) {
        err_set(rp, errno);
        return;
    }
    if (!S_ISDIR(st.st_mode)) {
        err_set(rp, ENOTDIR);
        return;
    }
    if (access(rp, R_OK | X_OK)) {
        err_set(rp, errno);
        return;
    }
    snprintf(g.cwd, sizeof g.cwd, "%s", rp);
    update_space();
    ti_set(&g.path, g.cwd);
    ti_set(&g.search, "");
    start_load(g.cwd, "");
    g.dirty = true;
}

void nav_up(void)
{
    char p[PATH_MAX];
    if (nav_at_root())
        return;
    path_parent(g.cwd, p, sizeof p);
    navigate(p);
}
