#include "fs/listing.h"
#include "app.h"
#include "util/path.h"
#include "util/utf8.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATCH 256

void list_clear(void)
{
    for (size_t i = 0; i < g.n; i++)
        free(g.v[i].name);
    g.n = 0;
    if (g.cap > 8192) {
        free(g.v);
        g.v = NULL;
        g.cap = 0;
    }
}

static int cmp_entry(const void *a, const void *b)
{
    const Entry *x = a, *y = b;
    if (x->is_dir != y->is_dir)
        return x->is_dir ? -1 : 1;
    return u8_casecmp(x->name, y->name);
}

static void push_batch(Job *j, Entry *b, int *nb)
{
    pthread_mutex_lock(&g.mtx);
    if (!atomic_load(&j->cancel)) {
        if (g.n + (size_t)*nb > g.cap) {
            size_t nc = g.cap ? g.cap * 2 : 1024;
            while (nc < g.n + (size_t)*nb)
                nc *= 2;
            Entry *nv = realloc(g.v, nc * sizeof *nv);
            if (nv) {
                g.v = nv;
                g.cap = nc;
            }
        }
        if (g.n + (size_t)*nb <= g.cap) {
            memcpy(g.v + g.n, b, (size_t)*nb * sizeof(Entry));
            g.n += (size_t)*nb;
            *nb = 0;
        }
    }
    pthread_mutex_unlock(&g.mtx);
    for (int i = 0; i < *nb; i++)
        free(b[i].name);
    *nb = 0;
    wake();
}

static void *load_thread(void *arg)
{
    Job *j = arg;
    Entry batch[BATCH];
    int nb = 0;
    DIR *d = opendir(j->path);
    if (!d) {
        post_err(j->path, errno);
    } else {
        struct dirent *de;
        while (!atomic_load(&j->cancel) && (de = readdir(d))) {
            const char *nm = de->d_name;
            if (nm[0] == '.' && (!nm[1] || (nm[1] == '.' && !nm[2])))
                continue;
            if (nm[0] == '.' && !j->hidden)
                continue;
            if (j->pat[0] && !u8_contains_ci(nm, j->pat))
                continue;
            char full[PATH_MAX];
            if (path_join(full, sizeof full, j->path, nm))
                continue;
            Entry e;
            memset(&e, 0, sizeof e);
            struct stat st;
            if (lstat(full, &st) == 0) {
                if (S_ISLNK(st.st_mode)) {
                    struct stat s2;
                    if (stat(full, &s2) == 0)
                        e.is_dir = S_ISDIR(s2.st_mode);
                } else {
                    e.is_dir = S_ISDIR(st.st_mode);
                }
                e.size = st.st_size;
                e.mtime = st.st_mtime;
                e.mode = st.st_mode;
            }
            e.name = strdup(nm);
            if (!e.name)
                continue;
            batch[nb++] = e;
            if (nb == BATCH)
                push_batch(j, batch, &nb);
        }
        closedir(d);
        if (nb)
            push_batch(j, batch, &nb);
    }
    pthread_mutex_lock(&g.mtx);
    if (!atomic_load(&j->cancel)) {
        if (g.n)
            qsort(g.v, g.n, sizeof(Entry), cmp_entry);
        for (size_t i = 0; i < g.n; i++)
            g.v[i].sel = false;
        g.loading = false;
    }
    if (g.job == j)
        g.job = NULL;
    pthread_mutex_unlock(&g.mtx);
    free(j);
    wake();
    return NULL;
}

/* main thread only, g.mtx held */
void start_load(const char *path, const char *pat)
{
    if (g.job)
        atomic_store(&g.job->cancel, true);
    list_clear();
    g.top = 0;
    g.anchor = -1;
    Job *j = calloc(1, sizeof *j);
    if (!j) {
        g.loading = false;
        err_set("calloc", ENOMEM);
        return;
    }
    atomic_init(&j->cancel, false);
    j->hidden = g.show_hidden;
    snprintf(j->path, sizeof j->path, "%s", path);
    snprintf(j->pat, sizeof j->pat, "%s", pat ? pat : "");
    g.job = j;
    g.loading = true;
    g.searching = j->pat[0] != 0;
    pthread_t t;
    int r = pthread_create(&t, NULL, load_thread, j);
    if (r) {
        g.job = NULL;
        g.loading = false;
        free(j);
        err_set("pthread_create", r);
    } else {
        pthread_detach(t);
    }
}
