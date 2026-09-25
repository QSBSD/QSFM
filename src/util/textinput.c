#include "util/textinput.h"
#include "util/utf8.h"
#include <stdio.h>
#include <string.h>

void ti_set(TI *t, const char *s)
{
    snprintf(t->buf, sizeof t->buf, "%s", s);
    t->len = (int)strlen(t->buf);
    t->cur = t->len;
    t->first = 0;
}

static int prev_char(const TI *t, int pos)
{
    do
        pos--;
    while (pos > 0 && (t->buf[pos] & 0xC0) == 0x80);
    return pos;
}

static int next_char(const TI *t, int pos)
{
    do
        pos++;
    while (pos < t->len && (t->buf[pos] & 0xC0) == 0x80);
    return pos;
}

int ti_cur_index(const TI *t)
{
    int n = 0;
    for (int i = 0; i < t->cur; i++)
        if ((t->buf[i] & 0xC0) != 0x80)
            n++;
    return n;
}

int ti_index_to_byte(const TI *t, int idx)
{
    int pos = 0;
    while (idx > 0 && pos < t->len) {
        pos = next_char(t, pos);
        idx--;
    }
    return pos;
}

static void ti_insert(TI *t, uint32_t cp)
{
    char e[4];
    int n = u8_encode(cp, e);
    if (t->len + n >= (int)sizeof t->buf)
        return;
    memmove(t->buf + t->cur + n, t->buf + t->cur, (size_t)(t->len - t->cur + 1));
    memcpy(t->buf + t->cur, e, (size_t)n);
    t->cur += n;
    t->len += n;
}

int ti_key(TI *t, uint32_t ks, uint32_t cp)
{
    switch (ks) {
    case 0xff08: /* BackSpace */
        if (t->cur > 0) {
            int p = prev_char(t, t->cur);
            memmove(t->buf + p, t->buf + t->cur, (size_t)(t->len - t->cur + 1));
            t->len -= t->cur - p;
            t->cur = p;
        }
        return 1;
    case 0xffff: /* Delete */
        if (t->cur < t->len) {
            int q = next_char(t, t->cur);
            memmove(t->buf + t->cur, t->buf + q, (size_t)(t->len - q + 1));
            t->len -= q - t->cur;
        }
        return 1;
    case 0xff51: /* Left */
        if (t->cur > 0) t->cur = prev_char(t, t->cur);
        return 1;
    case 0xff53: /* Right */
        if (t->cur < t->len) t->cur = next_char(t, t->cur);
        return 1;
    case 0xff50: /* Home */
        t->cur = 0;
        return 1;
    case 0xff57: /* End */
        t->cur = t->len;
        return 1;
    }
    if (cp) {
        ti_insert(t, cp);
        return 1;
    }
    return 0;
}
