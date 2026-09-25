#include "fs/launch.h"
#include "app.h"
#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

void open_file(const char *path)
{
    pid_t p = fork();
    if (p < 0) {
        err_set("fork", errno);
        return;
    }
    if (p == 0) {
        if (fork() != 0)
            _exit(0);
        setsid();
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2)
                close(fd);
        }
        execlp(cfg.opener, cfg.opener, path, (char *)NULL);
        _exit(127);
    }
    waitpid(p, NULL, 0);
}

/* runs: cfg.terminal <dir>, with dir as the working directory; main thread, g.mtx held */
void open_terminal(const char *dir)
{
    char prog[512];
    const char *home = getenv("HOME");
    if (!cfg.terminal[0]) {
        err_msg("Не задан параметр terminal в config.conf (~/.config/QSFM/config.conf)");
        return;
    }
    if (cfg.terminal[0] == '~' && (cfg.terminal[1] == '/' || !cfg.terminal[1]) && home)
        snprintf(prog, sizeof prog, "%s%s", home, cfg.terminal + 1);
    else
        snprintf(prog, sizeof prog, "%s", cfg.terminal);
    if (strchr(prog, '/') && access(prog, X_OK)) {
        err_set(prog, errno);
        return;
    }
    pid_t p = fork();
    if (p < 0) {
        err_set("fork", errno);
        return;
    }
    if (p == 0) {
        if (fork() != 0)
            _exit(0);
        setsid();
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2)
                close(fd);
        }
        if (chdir(dir))
            _exit(126);
        execlp(prog, prog, dir, (char *)NULL);
        _exit(127);
    }
    waitpid(p, NULL, 0);
}
