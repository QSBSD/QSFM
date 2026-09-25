#include "fs/delete.h"
#include "app.h"
#include "fs/opctx.h"
#include "fs/selection.h"
#include "util/path.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int rm_tree(const char *path, Ctx *c)
{
    struct stat st;
    if (lstat(path, &st)) {
        ctx_set(c, path, errno);
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) {
            ctx_set(c, path, errno);
            return -1;
        }
        struct dirent *de;
        while ((de = readdir(d))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
            char sub[PATH_MAX];
            if (path_join(sub, sizeof sub, path, de->d_name)) {
                closedir(d);
                ctx_set(c, path, ENAMETOOLONG);
                return -1;
            }
            if (rm_tree(sub, c)) {
                int e = errno;
                closedir(d);
                errno = e;
                return -1;
            }
        }
        closedir(d);
        if (rmdir(path)) {
            ctx_set(c, path, errno);
            return -1;
        }
        ctx_tick(c);
        return 0;
    }
    if (unlink(path)) {
        ctx_set(c, path, errno);
        return -1;
    }
    ctx_tick(c);
    return 0;
}

typedef struct {
    char **paths;
    int n;
} DelJob;

static void *delete_thread(void *arg)
{
    DelJob *j = arg;
    Ctx c;
    memset(&c, 0, sizeof c);
    for (int i = 0; i < j->n; i++) {
        if (rm_tree(j->paths[i], &c)) {
            post_err(c.ctx, errno);
            break;
        }
    }
    paths_free(j->paths, j->n);
    free(j);
    atomic_store(&g.busy, 0);
    atomic_store(&g.reload, 1);
    wake();
    return NULL;
}

/* main thread, g.mtx held; takes ownership of paths */
void start_delete(char **paths, int n)
{
    int exp = 0;
    if (!atomic_compare_exchange_strong(&g.busy, &exp, 1)) {
        err_msg("Операция уже выполняется");
        paths_free(paths, n);
        return;
    }
    DelJob *j = calloc(1, sizeof *j);
    if (!j) {
        atomic_store(&g.busy, 0);
        paths_free(paths, n);
        return;
    }
    j->paths = paths;
    j->n = n;
    atomic_store(&g.opkind, OP_DELETE);
    atomic_store(&g.done, 0);
    atomic_store(&g.total, 0);
    pthread_t t;
    int r = pthread_create(&t, NULL, delete_thread, j);
    if (r) {
        atomic_store(&g.busy, 0);
        paths_free(paths, n);
        free(j);
        err_set("pthread_create", r);
    } else {
        pthread_detach(t);
    }
}
