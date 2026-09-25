#ifndef SVGRENDER_H
#define SVGRENDER_H
#include <stdint.h>

typedef struct {
    int w, h;
    uint8_t *rgba;
} SvgBitmap;

int svg_render(const char *svg, int size, SvgBitmap *out);
void svg_free(SvgBitmap *b);
#endif
