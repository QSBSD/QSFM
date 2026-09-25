#ifndef FM_UTF8_H
#define FM_UTF8_H

#include <stdbool.h>
#include <stdint.h>

int u8_decode(const char *s, uint32_t *cp);
int u8_encode(uint32_t cp, char *out);
int u8_len(const char *s);
uint32_t cp_fold(uint32_t c);
int u8_casecmp(const char *a, const char *b);
bool u8_contains_ci(const char *hay, const char *needle);
#endif
