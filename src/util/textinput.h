#ifndef FM_TEXTINPUT_H
#define FM_TEXTINPUT_H

#include <stdint.h>

typedef struct {
    char buf[1024];
    int len, cur, first;
} TI;

void ti_set(TI *t, const char *s);
int ti_key(TI *t, uint32_t ks, uint32_t cp);
int ti_cur_index(const TI *t);
int ti_index_to_byte(const TI *t, int idx);
#endif
