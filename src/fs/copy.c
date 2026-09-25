#include "fs/copy.h"
#include "app.h"
#include "fs/delete.h"
#include "fs/opctx.h"
#include "fs/selection.h"
#include "util/path.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSZ (256 * 1024)

typedef struct {
    char **src;
    int n;
    bool move;
    char dst[PATH_MAX];
} CopyJob;

static long long walk_size(const char *path)
{
    struct stat st;
    if (lstat(path, &st))
        return 0;
    if (S_ISREG(st.st_mode))
        return st.st_size;
    if (!S_ISDIR(st.st_mode))
        return 0;
    long long sum = 0;
    DIR *d = opendir(path);
    if (!d)
        return 0;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        char sub[PATH_MAX];
        if (path_join(sub, sizeof sub, path, de->d_name) == 0)
            sum += walk_size(sub);
    }
    closedir(d);
    return sum;
}

static int copy_file(const char *src, const char *dst, mode_t mode, Ctx *c)
{
    int e = 0;
    int in = open(src, O_RDONLY);
    if (in < 0) {
        ctx_set(c, src, errno);
        return -1;
    }
    int out = open(dst, O_WRONLY | O_CREAT | O_EXCL, mode & 0777);
    if (out < 0) {
        e = errno;
        close(in);
        ctx_set(c, dst, e);
        return -1;
    }
    for (;;) {
        ssize_t r = read(in, c->buf, BUFSZ);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            e = errno;
            ctx_set(c, src, e);
            goto fail;
        }
        if (r == 0)
            break;
        for (ssize_t off = 0; off < r;) {
            ssize_t w = write(out, c->buf + off, (size_t)(r - off));
            if (w < 0) {
                if (errno == EINTR)
                    continue;
                e = errno;
                ctx_set(c, dst, e);
                goto fail;
            }
            off += w;
        }
        atomic_fetch_add(&g.done, r);
        ctx_tick(c);
    }
    close(in);
    if (close(out)) {
        e = errno;
        unlink(dst);
        ctx_set(c, dst, e);
        return -1;
    }
    return 0;
fail:
    close(in);
    close(out);
    unlink(dst);
    errno = e;
    return -1;
}

static int copy_tree(const char *src, const char *dst, Ctx *c)
{
    struct stat st;
    if (lstat(src, &st)) {
        ctx_set(c, src, errno);
        return -1;
    }
    if (S_ISLNK(st.st_mode)) {
        char tgt[PATH_MAX];
        ssize_t n = readlink(src, tgt, sizeof tgt - 1);
        if (n < 0) {
            ctx_set(c, src, errno);
            return -1;
        }
        tgt[n] = 0;
        if (symlink(tgt, dst)) {
            ctx_set(c, dst, errno);
            return -1;
        }
        return 0;
    }
    if (S_ISREG(st.st_mode))
        return copy_file(src, dst, st.st_mode, c);
    if (!S_ISDIR(st.st_mode))
        return 0;
    if (mkdir(dst, (st.st_mode & 0777) | 0700)) {
        ctx_set(c, dst, errno);
        return -1;
    }
    DIR *d = opendir(src);
    if (!d) {
        ctx_set(c, src, errno);
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        char s2[PATH_MAX], d2[PATH_MAX];
        if (path_join(s2, sizeof s2, src, de->d_name) || path_join(d2, sizeof d2, dst, de->d_name)) {
            closedir(d);
            ctx_set(c, src, ENAMETOOLONG);
            return -1;
        }
        if (copy_tree(s2, d2, c)) {
            int e = errno;
            closedir(d);
            errno = e;
            return -1;
        }
    }
    closedir(d);
    chmod(dst, st.st_mode & 0777);
    return 0;
}

/* name conflict: "name (копия).ext", "name (копия 2).ext", ... */
static int unique_dst(const char *dir, const char *name, bool isdir, char *out, size_t n)
{
    struct stat st;
    if (path_join(out, n, dir, name))
        return -1;
    if (lstat(out, &st) != 0)
        return 0;
    char stem[512], ext[512];
    const char *dot = isdir ? NULL : strrchr(name, '.');
    if (dot && dot != name) {
        snprintf(stem, sizeof stem, "%.*s", (int)(dot - name), name);
        snprintf(ext, sizeof ext, "%s", dot);
    } else {
        snprintf(stem, sizeof stem, "%s", name);
        ext[0] = 0;
    }
    for (int k = 1; k < 10000; k++) {
        char cand[1100];
        if (k == 1)
            snprintf(cand, sizeof cand, "%s (копия)%s", stem, ext);
        else
            snprintf(cand, sizeof cand, "%s (копия %d)%s", stem, k, ext);
        if (path_join(out, n, dir, cand))
            return -1;
        if (lstat(out, &st) != 0)
            return 0;
    }
    errno = EEXIST;
    return -1;
}

static void *copy_thread(void *arg)
{
    CopyJob *j = arg;
    Ctx c;
    memset(&c, 0, sizeof c);
    c.buf = malloc(BUFSZ);
    if (!c.buf) {
        post_err("malloc", ENOMEM);
    } else {
        long long total = 0;
        for (int i = 0; i < j->n; i++)
            total += walk_size(j->src[i]);
        atomic_store(&g.done, 0);
        atomic_store(&g.total, total);
        wake();
        for (int i = 0; i < j->n; i++) {
            struct stat st;
            char dst[PATH_MAX];
            if (lstat(j->src[i], &st)) {
                post_err(j->src[i], errno);
                break;
            }
            if (S_ISDIR(st.st_mode)) {
                char rs[PATH_MAX], rd[PATH_MAX];
                if (realpath(j->src[i], rs) && realpath(j->dst, rd)) {
                    size_t l = strlen(rs);
                    if (strncmp(rd, rs, l) == 0 && (rd[l] == '/' || rd[l] == 0)) {
                        post_msg(j->move ? "Нельзя переместить папку в саму себя" : "Нельзя копировать папку в саму себя");
                        break;
                    }
                }
            }
            if (j->move) {
                char par[PATH_MAX];
                path_parent(j->src[i], par, sizeof par);
                if (strcmp(par, j->dst) == 0)
                    continue; /* already here */
            }
            if (unique_dst(j->dst, path_base(j->src[i]), S_ISDIR(st.st_mode), dst, sizeof dst)) {
                post_err(j->src[i], errno);
                break;
            }
            if (j->move && rename(j->src[i], dst) == 0)
                continue;
            if (j->move && errno != EXDEV) {
                post_err(j->src[i], errno);
                break;
            }
            if (copy_tree(j->src[i], dst, &c)) {
                post_err(c.ctx, errno);
                break;
            }
            if (j->move && rm_tree(j->src[i], &c)) {
                post_err(c.ctx, errno);
                break;
            }
        }
        free(c.buf);
    }
    paths_free(j->src, j->n);
    free(j);
    atomic_store(&g.busy, 0);
    atomic_store(&g.reload, 1);
    wake();
    return NULL;
}

/* main thread, g.mtx held; takes ownership of paths */
static void start_job(char **paths, int n, const char *dst, bool move)
{
    int exp = 0;
    if (!atomic_compare_exchange_strong(&g.busy, &exp, 1)) {
        err_msg("Операция уже выполняется");
        paths_free(paths, n);
        return;
    }
    CopyJob *j = calloc(1, sizeof *j);
    if (!j) {
        atomic_store(&g.busy, 0);
        paths_free(paths, n);
        return;
    }
    j->src = paths;
    j->n = n;
    j->move = move;
    snprintf(j->dst, sizeof j->dst, "%s", dst);
    atomic_store(&g.opkind, OP_COPY);
    atomic_store(&g.done, 0);
    atomic_store(&g.total, 0);
    pthread_t t;
    int r = pthread_create(&t, NULL, copy_thread, j);
    if (r) {
        atomic_store(&g.busy, 0);
        paths_free(paths, n);
        free(j);
        err_set("pthread_create", r);
    } else {
        pthread_detach(t);
    }
}

void start_copy(char **paths, int n, const char *dst)
{
    start_job(paths, n, dst, false);
}

void start_move(char **paths, int n, const char *dst)
{
    start_job(paths, n, dst, true);
}
