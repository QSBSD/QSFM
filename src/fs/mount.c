#include "fs/mount.h"
#include "app.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char op;
    char mp[512];
} MntJob;

static void *mount_thread(void *arg)
{
    MntJob *m = arg;
    const char *cmd = m->op == 'm' ? "mount" : "umount";
    int pf[2];
    if (pipe(pf)) {
        post_err(cmd, errno);
    } else {
        pid_t p = fork();
        if (p < 0) {
            post_err("fork", errno);
            close(pf[0]);
            close(pf[1]);
        } else if (p == 0) {
            dup2(pf[1], 1);
            dup2(pf[1], 2);
            close(pf[0]);
            close(pf[1]);
            execlp(cmd, cmd, m->mp, (char *)NULL);
            _exit(127);
        } else {
            char out[300];
            size_t got = 0;
            ssize_t r;
            close(pf[1]);
            while (got < sizeof out - 1 && (r = read(pf[0], out + got, sizeof out - 1 - got)) > 0)
                got += (size_t)r;
            out[got] = 0;
            close(pf[0]);
            int st = 0;
            waitpid(p, &st, 0);
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                char msg[400];
                snprintf(msg, sizeof msg, "%s %s:\n%s", cmd, m->mp, got ? out : "ошибка");
                post_msg(msg);
            } else if (m->op == 'm') {
                pthread_mutex_lock(&g.mtx);
                snprintf(g.nav_path, sizeof g.nav_path, "%s", m->mp);
                pthread_mutex_unlock(&g.mtx);
            }
        }
    }
    atomic_store(&g.places_dirty, 1);
    atomic_store(&g.busy, 0);
    free(m);
    wake();
    return NULL;
}

/* main thread, g.mtx held */
void start_mount(const char *mp, char op)
{
    int exp = 0;
    if (!atomic_compare_exchange_strong(&g.busy, &exp, 1)) {
        err_msg("Операция уже выполняется");
        return;
    }
    MntJob *m = calloc(1, sizeof *m);
    if (!m) {
        atomic_store(&g.busy, 0);
        return;
    }
    m->op = op;
    snprintf(m->mp, sizeof m->mp, "%s", mp);
    atomic_store(&g.opkind, OP_MOUNT);
    pthread_t t;
    int r = pthread_create(&t, NULL, mount_thread, m);
    if (r) {
        atomic_store(&g.busy, 0);
        free(m);
        err_set("pthread_create", r);
    } else {
        pthread_detach(t);
    }
}
