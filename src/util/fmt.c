#include "util/fmt.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void human_size(long long b, char *out, size_t n)
{
    static const char *u[] = {"Б", "КБ", "МБ", "ГБ", "ТБ"};
    double v = (double)b;
    int i = 0;
    while (v >= 1024.0 && i < 4) {
        v /= 1024.0;
        i++;
    }
    if (i == 0)
        snprintf(out, n, "%lld %s", b, u[0]);
    else
        snprintf(out, n, "%.1f %s", v, u[i]);
}

void fmt_time(time_t t, char *out, size_t n)
{
    struct tm tm;
    if (!localtime_r(&t, &tm)) {
        snprintf(out, n, "?");
        return;
    }
    if (!strftime(out, n, cfg.date_format, &tm))
        out[0] = 0;
}

void fmt_perms(mode_t m, char *sym, char *oct, size_t n)
{
    static const char rwx[] = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
        sym[i] = (m & (0400u >> i)) ? rwx[i] : '-';
    if (m & S_ISUID) sym[2] = (m & S_IXUSR) ? 's' : 'S';
    if (m & S_ISGID) sym[5] = (m & S_IXGRP) ? 's' : 'S';
    if (m & S_ISVTX) sym[8] = (m & S_IXOTH) ? 't' : 'T';
    sym[9] = 0;
    snprintf(oct, n, "%04o", (unsigned)(m & 07777));
}
