#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#include "svg/svgrender.h"

int svg_render(const char *svg, int size, SvgBitmap *out)
{
    memset(out, 0, sizeof *out);
    char *copy = strdup(svg);
    if (!copy)
        return -1;
    NSVGimage *img = nsvgParse(copy, "px", 96.0f);
    free(copy);
    if (!img)
        return -1;
    if (img->width <= 0 || img->height <= 0) {
        nsvgDelete(img);
        return -1;
    }
    float m = img->width > img->height ? img->width : img->height;
    float sc = (float)size / m;
    int w = (int)(img->width * sc + 0.5f);
    int h = (int)(img->height * sc + 0.5f);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    NSVGrasterizer *r = nsvgCreateRasterizer();
    uint8_t *buf = calloc((size_t)w * h, 4);
    if (!r || !buf) {
        if (r) nsvgDeleteRasterizer(r);
        free(buf);
        nsvgDelete(img);
        return -1;
    }
    nsvgRasterize(r, img, 0, 0, sc, buf, w, h, w * 4);
    nsvgDeleteRasterizer(r);
    nsvgDelete(img);
    out->w = w;
    out->h = h;
    out->rgba = buf;
    return 0;
}

void svg_free(SvgBitmap *b)
{
    free(b->rgba);
    memset(b, 0, sizeof *b);
}
