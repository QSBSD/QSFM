#ifndef FM_FMT_H
#define FM_FMT_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

void human_size(long long b, char *out, size_t n);
void fmt_time(time_t t, char *out, size_t n);
void fmt_perms(mode_t m, char *sym, char *oct, size_t n);
#endif
