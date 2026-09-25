#include "config.h"
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

Config cfg;

static char warns[1024];

void config_warn(const char *fmt, ...)
{
    char m[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(m, sizeof m, fmt, ap);
    va_end(ap);
    fprintf(stderr, "QSFM: %s\n", m);
    size_t n = strlen(warns);
    if (n + strlen(m) + 2 < sizeof warns)
        snprintf(warns + n, sizeof warns - n, "%s%s", n ? "\n" : "", m);
}

const char *config_warnings(void)
{
    return warns;
}

typedef enum { T_DOUBLE, T_INT, T_BOOL, T_STR, T_COLOR } OptType;

typedef struct {
    const char *key;
    OptType type;
    void *ptr;
    size_t size;
    double lo, hi;
    const char *doc;
} Opt;

static const Opt opts[] = {
    {"scale", T_DOUBLE, &cfg.scale, 0, 0.5, 10.0,
     "Масштаб интерфейса: 1.0 = 100%, 1.5 = 150%, 2.0 = 200%. Масштабируются размеры элементов, значки,\n"
     "размер окна и шрифт."},
    {"font", T_STR, cfg.font, sizeof cfg.font, 0, 0,
     "Шрифт: имя для fontconfig (список: fc-list :spacing=mono family style) или путь к файлу .ttf/.otf.\n"
     "Примеры: monospace, JetBrains Mono, JetBrains Mono NL:style=ExtraLight, ~/fonts/Mono.ttf. Шрифт должен\n"
     "быть моноширинным."},
    {"font_size", T_INT, &cfg.font_size, 0, 6, 72, "Размер шрифта в пикселях при scale = 1.0."},
    {"window_width", T_INT, &cfg.window_width, 0, 300, 10000, "Ширина окна при запуске (до масштабирования)."},
    {"window_height", T_INT, &cfg.window_height, 0, 200, 10000, "Высота окна при запуске (до масштабирования)."},
    {"sidebar_width", T_INT, &cfg.sidebar_width, 0, 100, 1000, "Ширина боковой панели (до масштабирования)."},
    {"icon_size", T_INT, &cfg.icon_size, 0, 16, 256, "Размер значков файлов и папок (до масштабирования)."},
    {"place_icon_size", T_INT, &cfg.place_icon_size, 0, 8, 128,
     "Размер значков в боковой панели «Места» (до масштабирования)."},
    {"up_icon_size", T_INT, &cfg.up_icon_size, 0, 8, 128, "Размер значка кнопки «вверх» (до масштабирования)."},
    {"cell_width", T_INT, &cfg.cell_width, 0, 48, 400, "Ширина ячейки файла в сетке (до масштабирования)."},
    {"show_hidden", T_BOOL, &cfg.show_hidden, 0, 0, 0, "Показывать скрытые файлы при запуске (Ctrl+H переключает)."},
    {"scroll_lines", T_INT, &cfg.scroll_lines, 0, 1, 50, "Рядов сетки за один шаг колеса мыши."},
    {"date_format", T_STR, cfg.date_format, sizeof cfg.date_format, 0, 0, "Формат даты в свойствах (strftime)."},
    {"start_dir", T_STR, cfg.start_dir, sizeof cfg.start_dir, 0, 0, "Папка при запуске (~ = домашняя)."},
    {"opener", T_STR, cfg.opener, sizeof cfg.opener, 0, 0, "Программа для открытия файлов (вызывается как: opener путь)."},
    {"terminal", T_STR, cfg.terminal, sizeof cfg.terminal, 0, 1,
     "Скрипт или программа для пункта «Открыть в терминале»: вызывается как «terminal путь_к_папке»,\n"
     "рабочая папка — эта папка. Пример: ~/bin/term.sh. Пусто — пункт выдаёт подсказку."},

    {"color_bg", T_COLOR, &cfg.c_bg, 0, 0, 0, "Цвета: #rrggbb. Фон списка файлов и полей ввода."},
    {"color_toolbar", T_COLOR, &cfg.c_toolbar, 0, 0, 0, "Фон верхней панели и строки состояния."},
    {"color_sidebar", T_COLOR, &cfg.c_sidebar, 0, 0, 0, "Фон боковой панели и диалогов."},
    {"color_border", T_COLOR, &cfg.c_border, 0, 0, 0, "Линии и рамки."},
    {"color_text", T_COLOR, &cfg.c_text, 0, 0, 0, "Текст."},
    {"color_text_dim", T_COLOR, &cfg.c_text_dim, 0, 0, 0, "Неактивный текст, подсказки."},
    {"color_selection", T_COLOR, &cfg.c_selection, 0, 0, 0, "Фон выделения, пункта меню, заголовка диалога."},
    {"color_selection_text", T_COLOR, &cfg.c_selection_text, 0, 0, 0, "Текст и значки на выделении."},
    {"color_place_selection", T_COLOR, &cfg.c_place_selection, 0, 0, 0, "Фон текущего места в боковой панели."},
    {"color_focus", T_COLOR, &cfg.c_focus, 0, 0, 0, "Рамка активного поля ввода."},
    {"color_button", T_COLOR, &cfg.c_button, 0, 0, 0, "Фон кнопок."},
    {"color_menu", T_COLOR, &cfg.c_menu, 0, 0, 0, "Фон контекстного меню."},
    {"color_scrollbar", T_COLOR, &cfg.c_scrollbar, 0, 0, 0, "Полоса прокрутки и индикатор прогресса."},
    {"color_scrollbar_thumb", T_COLOR, &cfg.c_scrollbar_thumb, 0, 0, 0, "Ползунок прокрутки."},
    {"color_icon", T_COLOR, &cfg.c_icon, 0, 0, 0, "Значки."},
    {"color_icon_dim", T_COLOR, &cfg.c_icon_dim, 0, 0, 0, "Неактивные значки."},
};

#define NOPTS (sizeof opts / sizeof *opts)

static void defaults(void)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.scale = 1.0;
    snprintf(cfg.font, sizeof cfg.font, "%s", "monospace");
    cfg.font_size = 13;
    cfg.window_width = 900;
    cfg.window_height = 600;
    cfg.sidebar_width = 190;
    cfg.icon_size = 48;
    cfg.place_icon_size = 16;
    cfg.up_icon_size = 20;
    cfg.cell_width = 96;
    cfg.show_hidden = false;
    cfg.scroll_lines = 1;
    snprintf(cfg.date_format, sizeof cfg.date_format, "%s", "%Y-%m-%d %H:%M");
    snprintf(cfg.start_dir, sizeof cfg.start_dir, "%s", "~");
    snprintf(cfg.opener, sizeof cfg.opener, "%s", "xdg-open");

    cfg.c_bg = 0x000000;
    cfg.c_toolbar = 0x000000;
    cfg.c_sidebar = 0x000000;
    cfg.c_border = 0xffffff;
    cfg.c_text = 0xffffff;
    cfg.c_text_dim = 0x808080;
    cfg.c_selection = 0xffffff;
    cfg.c_selection_text = 0x000000;
    cfg.c_place_selection = 0x303030;
    cfg.c_focus = 0xffffff;
    cfg.c_button = 0x000000;
    cfg.c_menu = 0x000000;
    cfg.c_scrollbar = 0x202020;
    cfg.c_scrollbar_thumb = 0xffffff;
    cfg.c_icon = 0xffffff;
    cfg.c_icon_dim = 0x808080;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1]))
        s[--n] = 0;
    return s;
}

static bool parse_value(const Opt *o, const char *v)
{
    char *end;
    switch (o->type) {
    case T_DOUBLE: {
        double d = strtod(v, &end);
        if (end == v || *end || d < o->lo || d > o->hi)
            return false;
        *(double *)o->ptr = d;
        return true;
    }
    case T_INT: {
        long n = strtol(v, &end, 10);
        if (end == v || *end || n < (long)o->lo || n > (long)o->hi)
            return false;
        *(int *)o->ptr = (int)n;
        return true;
    }
    case T_BOOL:
        if (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || !strcasecmp(v, "on") || !strcmp(v, "1"))
            *(bool *)o->ptr = true;
        else if (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcasecmp(v, "off") || !strcmp(v, "0"))
            *(bool *)o->ptr = false;
        else
            return false;
        return true;
    case T_STR:
        if ((!*v && o->hi != 1) || strlen(v) >= o->size) /* hi == 1: empty value allowed */
            return false;
        snprintf((char *)o->ptr, o->size, "%s", v);
        return true;
    case T_COLOR: {
        if (*v == '#')
            v++;
        else if (v[0] == '0' && (v[1] == 'x' || v[1] == 'X'))
            v += 2;
        if (strlen(v) != 6)
            return false;
        unsigned long c = strtoul(v, &end, 16);
        if (*end)
            return false;
        *(uint32_t *)o->ptr = (uint32_t)c;
        return true;
    }
    }
    return false;
}

static void write_default(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "# QSFM: настройки. Формат: параметр = значение.\n"
               "# Комментарии — только на отдельной строке (начинаются с #).\n"
               "# Пропущенные параметры принимают значения по умолчанию.\n");
    for (size_t i = 0; i < NOPTS; i++) {
        const Opt *o = &opts[i];
        if (i == 0 || (o->type == T_COLOR && opts[i - 1].type != T_COLOR) || !strcmp(o->key, "window_width") ||
            !strcmp(o->key, "show_hidden"))
            fputc('\n', f);
        const char *d = o->doc;
        while (*d) {
            const char *nl = strchr(d, '\n');
            int len = nl ? (int)(nl - d) : (int)strlen(d);
            fprintf(f, "# %.*s\n", len, d);
            d += len + (nl ? 1 : 0);
        }
        fprintf(f, "%s = ", o->key);
        switch (o->type) {
        case T_DOUBLE: fprintf(f, "%.2f", *(double *)o->ptr); break;
        case T_INT: fprintf(f, "%d", *(int *)o->ptr); break;
        case T_BOOL: fprintf(f, "%s", *(bool *)o->ptr ? "true" : "false"); break;
        case T_STR: fprintf(f, "%s", (char *)o->ptr); break;
        case T_COLOR: fprintf(f, "#%06x", (unsigned)*(uint32_t *)o->ptr); break;
        }
        fputc('\n', f);
    }
    fclose(f);
}

static void ensure_dir(const char *p)
{
    if (mkdir(p, 0755) && errno != EEXIST)
        return;
}

void config_load(void)
{
    defaults();
    const char *xdg = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME");
    char base[512], dir[600], path[700];
    if (xdg && xdg[0] == '/')
        snprintf(base, sizeof base, "%s", xdg);
    else if (home && *home)
        snprintf(base, sizeof base, "%s/.config", home);
    else
        return;
    snprintf(dir, sizeof dir, "%s/QSFM", base);
    snprintf(path, sizeof path, "%s/config.conf", dir);

    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) {
            ensure_dir(base);
            ensure_dir(dir);
            write_default(path);
        }
        return;
    }
    char line[1024];
    int ln = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        char *s = trim(line);
        if (!*s || *s == '#' || *s == ';')
            continue;
        char *eq = strchr(s, '=');
        if (!eq) {
            config_warn("config.conf, строка %d: ожидается «параметр = значение».", ln);
            continue;
        }
        *eq = 0;
        char *k = trim(s), *v = trim(eq + 1);
        size_t i;
        for (i = 0; i < NOPTS; i++)
            if (!strcmp(k, opts[i].key))
                break;
        if (i == NOPTS && (!strcmp(k, "row_height") || !strcmp(k, "color_header") || !strcmp(k, "double_click_ms")))
            continue; /* removed options */
        if (i == NOPTS)
            config_warn("config.conf, строка %d: неизвестный параметр «%s».", ln, k);
        else {
            if (opts[i].type != T_STR) { /* inline comment: " # ..." after the value */
                for (char *c = v + (*v == '#'); *c; c++)
                    if (*c == '#' && c > v && isspace((unsigned char)c[-1])) {
                        *c = 0;
                        v = trim(v);
                        break;
                    }
            }
            if (!parse_value(&opts[i], v))
                config_warn("config.conf, строка %d: недопустимое значение «%s» для «%s» (значение по умолчанию).", ln, v, k);
        }
    }
    fclose(f);
}
