#include "fs/places.h"
#include "app.h"
#include <ctype.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

static void place_add(const char *label, const char *path, int kind, bool mounted)
{
    if (g.nplaces >= MAX_PLACES)
        return;
    Place *p = &g.places[g.nplaces++];
    snprintf(p->label, sizeof p->label, "%s", label);
    snprintf(p->path, sizeof p->path, "%s", path);
    p->kind = kind;
    p->mounted = mounted;
}

/* last path component: "/mnt/data/" -> "data" */
static const char *place_label(const char *path)
{
    static char buf[64];
    size_t n = strlen(path);
    while (n > 1 && path[n - 1] == '/')
        n--;
    size_t st = n;
    while (st > 0 && path[st - 1] != '/')
        st--;
    if (st == n) {
        snprintf(buf, sizeof buf, "%s", path);
        return buf;
    }
    snprintf(buf, sizeof buf, "%.*s", (int)(n - st), path + st);
    return buf;
}

static bool place_has(const char *path)
{
    for (int i = 0; i < g.nplaces; i++)
        if (strcmp(g.places[i].path, path) == 0)
            return true;
    return false;
}

#if !defined(__FreeBSD__)
static void mnt_unescape(char *s)
{
    char *o = s;
    while (*s) {
        if (s[0] == '\\' && isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3])) {
            *o++ = (char)(((s[1] - '0') << 6) | ((s[2] - '0') << 3) | (s[3] - '0'));
            s += 4;
        } else {
            *o++ = *s++;
        }
    }
    *o = 0;
}
#endif

void places_refresh(void)
{
    const char *home = getenv("HOME");
    if (!home || !*home)
        home = "/";
    char desk[512];
    g.nplaces = 0;
    place_add(place_label(home), home, 0, true);
    snprintf(desk, sizeof desk, "%s/Desktop", home);
    place_add(place_label(desk), desk, 1, true);
    place_add("Корень (/)", "/", 2, true);
#if defined(__FreeBSD__)
    struct statfs *fs = NULL;
    int cnt = getmntinfo(&fs, MNT_NOWAIT);
    for (int i = 0; i < cnt; i++) {
        if (strncmp(fs[i].f_mntfromname, "/dev/", 5) != 0)
            continue;
        if (!place_has(fs[i].f_mntonname))
            place_add(place_label(fs[i].f_mntonname), fs[i].f_mntonname, 2, true);
    }
#else
    FILE *f = fopen("/proc/mounts", "r");
    if (f) {
        char line[1024], dev[256], mp[512];
        while (fgets(line, sizeof line, f)) {
            if (sscanf(line, "%255s %511s", dev, mp) != 2)
                continue;
            mnt_unescape(mp);
            if (strncmp(dev, "/dev/", 5) != 0 || strncmp(dev, "/dev/loop", 9) == 0)
                continue;
            if (!place_has(mp))
                place_add(place_label(mp), mp, 2, true);
        }
        fclose(f);
    }
#endif
    struct fstab *fe;
    setfsent();
    while ((fe = getfsent()) != NULL) {
        if (!fe->fs_spec || !fe->fs_file || fe->fs_file[0] != '/')
            continue;
        if (strcmp(fe->fs_vfstype, "swap") == 0 || strcmp(fe->fs_type, "sw") == 0)
            continue;
        if (strncmp(fe->fs_spec, "/dev/", 5) && strncmp(fe->fs_spec, "UUID=", 5) && strncmp(fe->fs_spec, "LABEL=", 6) &&
            strncmp(fe->fs_spec, "PARTUUID=", 9) && strncmp(fe->fs_spec, "PARTLABEL=", 10))
            continue;
        if (!place_has(fe->fs_file))
            place_add(place_label(fe->fs_file), fe->fs_file, 2, false);
    }
    endfsent();
}
