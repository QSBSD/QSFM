#include "ui/actions.h"
#include "app.h"
#include "fs/launch.h"
#include "fs/selection.h"
#include "ui/dialog.h"
#include "ui/nav.h"
#include "util/fmt.h"
#include "util/path.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void ask_delete(void)
{
    int n;
    char **p = paths_selected(&n);
    char msg[PATH_MAX + 128];
    if (!p)
        return;
    if (n == 1)
        snprintf(msg, sizeof msg, "Удалить безвозвратно «%s»?\nДействие необратимо.", path_base(p[0]));
    else
        snprintf(msg, sizeof msg, "Удалить безвозвратно выбранное (%d шт.)?\nДействие необратимо.", n);
    dlg_open(DK_CONFIRM, DA_DELETE, "Удаление", msg, NULL, NULL);
    dlg.paths = p;
    dlg.npaths = n;
}

void ask_rename(void)
{
    int n;
    char **p = paths_selected(&n);
    if (!p)
        return;
    if (n == 1)
        dlg_open(DK_INPUT, DA_RENAME, "Переименование", "Новое имя:", path_base(p[0]), p[0]);
    paths_free(p, n);
}

static const char *type_name(mode_t m)
{
    if (S_ISDIR(m)) return "Каталог";
    if (S_ISREG(m)) return "Обычный файл";
    if (S_ISLNK(m)) return "Символьная ссылка";
    if (S_ISBLK(m)) return "Блочное устройство";
    if (S_ISCHR(m)) return "Символьное устройство";
    if (S_ISFIFO(m)) return "FIFO";
    if (S_ISSOCK(m)) return "Сокет";
    return "Неизвестно";
}

static void show_props(const char *path)
{
    struct stat st;
    if (lstat(path, &st)) {
        err_set(path, errno);
        return;
    }
    char sz[64], tm[192], sym[16], oct[8], dir[PATH_MAX], msg[PATH_MAX * 2 + 512];
    if (S_ISDIR(st.st_mode)) {
        snprintf(sz, sizeof sz, "—");
    } else {
        char h[32];
        human_size(st.st_size, h, sizeof h);
        snprintf(sz, sizeof sz, "%s (%lld байт)", h, (long long)st.st_size);
    }
    fmt_time(st.st_mtime, tm, sizeof tm);
    fmt_perms(st.st_mode, sym, oct, sizeof oct);
    path_parent(path, dir, sizeof dir);
    snprintf(msg, sizeof msg, "Имя: %s\nТип: %s\nРазмер: %s\nИзменён: %s\nПрава: %s (%s)\nРасположение: %s",
             strcmp(path, "/") ? path_base(path) : "/", type_name(st.st_mode), sz, tm, sym, oct, dir);
    dlg_open(DK_PROPS, DA_NONE, "Свойства", msg, NULL, path);
}

void ask_props(void)
{
    int n;
    char **p = paths_selected(&n);
    if (p && n == 1)
        show_props(p[0]);
    else
        show_props(g.cwd);
    if (p)
        paths_free(p, n);
}

void open_selected(void)
{
    int n;
    char **p = paths_selected(&n);
    if (!p)
        return;
    for (size_t i = 0; i < g.n; i++) {
        if (!g.v[i].sel)
            continue;
        if (g.v[i].is_dir) {
            if (n == 1) {
                char full[PATH_MAX];
                path_join(full, sizeof full, g.cwd, g.v[i].name);
                paths_free(p, n);
                navigate(full);
                return;
            }
        } else {
            char full[PATH_MAX];
            path_join(full, sizeof full, g.cwd, g.v[i].name);
            open_file(full);
        }
    }
    paths_free(p, n);
}

void open_terminal_selected(void)
{
    for (size_t i = 0; i < g.n; i++) {
        if (g.v[i].sel && g.v[i].is_dir) {
            char full[PATH_MAX];
            if (path_join(full, sizeof full, g.cwd, g.v[i].name) == 0)
                open_terminal(full);
            return;
        }
    }
}
