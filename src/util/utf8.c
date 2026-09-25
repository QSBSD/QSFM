#include "util/utf8.h"
#include <stdlib.h>
#include <string.h>

int u8_decode(const char *s, uint32_t *cp)
{
    const unsigned char *p = (const unsigned char *)s;
    if (!p[0]) {
        *cp = 0;
        return 0;
    }
    if (p[0] < 0x80) {
        *cp = p[0];
        return 1;
    }
    if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
        *cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        return 2;
    }
    if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        return 3;
    }
    if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
        *cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        return 4;
    }
    *cp = 0xFFFD;
    return 1;
}

int u8_encode(uint32_t cp, char *o)
{
    if (cp < 0x80) {
        o[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        o[0] = (char)(0xC0 | (cp >> 6));
        o[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        o[0] = (char)(0xE0 | (cp >> 12));
        o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        o[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    o[0] = (char)(0xF0 | (cp >> 18));
    o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    o[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    o[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

int u8_len(const char *s)
{
    int n = 0;
    uint32_t cp;
    int k;
    while ((k = u8_decode(s, &cp)) > 0) {
        s += k;
        n++;
    }
    return n;
}

uint32_t cp_fold(uint32_t c)
{
    if (c >= 'A' && c <= 'Z') return c + 32;
    if (c >= 0x410 && c <= 0x42F) return c + 32;
    if (c >= 0x400 && c <= 0x40F) return c + 80;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 32;
    return c;
}

int u8_casecmp(const char *a, const char *b)
{
    for (;;) {
        uint32_t x, y;
        int ka = u8_decode(a, &x), kb = u8_decode(b, &y);
        if (!ka || !kb)
            return ka ? 1 : (kb ? -1 : 0);
        x = cp_fold(x);
        y = cp_fold(y);
        if (x != y)
            return x < y ? -1 : 1;
        a += ka;
        b += kb;
    }
}

bool u8_contains_ci(const char *hay, const char *needle)
{
    uint32_t nd[256], hs[512], cp;
    int nn = 0, nh = 0, k;
    while (nn < 256 && (k = u8_decode(needle, &cp)) > 0) {
        nd[nn++] = cp_fold(cp);
        needle += k;
    }
    if (nn == 0)
        return true;
    while (nh < 512 && (k = u8_decode(hay, &cp)) > 0) {
        hs[nh++] = cp_fold(cp);
        hay += k;
    }
    for (int i = 0; i + nn <= nh; i++) {
        int j = 0;
        while (j < nn && hs[i + j] == nd[j])
            j++;
        if (j == nn)
            return true;
    }
    return false;
}
