#include "ui/icons.h"
#include "app.h"
#include "config.h"
#include "ui/theme.h"
#include "svg/svgrender.h"
#include <xcb/xcb_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SVG_HEAD "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"

/* '@' is replaced with the icon colour */
static const char *svg_src[IC_COUNT] = {
    SVG_HEAD "<path d=\"M2 5h7l2 2h11v3H2z\" fill=\"@\"/><path d=\"M2 11h20v9H2z\" fill=\"@\"/></svg>",
    SVG_HEAD "<path d=\"M5 2h9l5 5v15H5z\" fill=\"none\" stroke=\"@\" stroke-width=\"1.6\"/>"
             "<path d=\"M14 2v5h5\" fill=\"none\" stroke=\"@\" stroke-width=\"1.6\"/></svg>",
    SVG_HEAD "<path d=\"M12 3l8 9h-5v9H9v-9H4z\" fill=\"@\"/></svg>",
    SVG_HEAD "<path d=\"M12 3l8 9h-5v9H9v-9H4z\" fill=\"@\"/></svg>",
    SVG_HEAD "<path d=\"M12 3l10 9h-3v9h-5v-6h-4v6H5v-9H2z\" fill=\"@\"/></svg>",
    SVG_HEAD "<rect x=\"2.9\" y=\"4.9\" width=\"18.2\" height=\"11.2\" rx=\"1\" fill=\"none\" stroke=\"@\" stroke-width=\"1.8\"/>"
             "<rect x=\"9\" y=\"18\" width=\"6\" height=\"2\" fill=\"@\"/>"
             "<rect x=\"6\" y=\"20\" width=\"12\" height=\"1.5\" fill=\"@\"/></svg>",
    SVG_HEAD "<rect x=\"2.9\" y=\"8.9\" width=\"18.2\" height=\"8.2\" rx=\"2\" fill=\"none\" stroke=\"@\" stroke-width=\"1.8\"/>"
             "<rect x=\"5.5\" y=\"11.5\" width=\"7\" height=\"3\" fill=\"@\"/>"
             "<circle cx=\"18\" cy=\"13\" r=\"1.5\" fill=\"@\"/></svg>",
    SVG_HEAD "<rect x=\"2.9\" y=\"8.9\" width=\"18.2\" height=\"8.2\" rx=\"2\" fill=\"none\" stroke=\"@\" stroke-width=\"1.8\"/>"
             "<rect x=\"5.5\" y=\"11.5\" width=\"7\" height=\"3\" fill=\"@\"/>"
             "<circle cx=\"18\" cy=\"13\" r=\"1.5\" fill=\"@\"/></svg>",
};

/* colour: 0 = normal, 1 = dim, 2 = on selection */
static const int svg_col[IC_COUNT] = {0, 0, 0, 1, 0, 0, 0, 1};
static SvgBitmap icons[IC_COUNT];
static SvgBitmap icons_sel[IC_COUNT];

static void render(const char *tpl, int size, uint32_t col, SvgBitmap *out)
{
    char c[8], *svg = malloc(strlen(tpl) * 8 + 1), *o = svg;
    if (!svg)
        return;
    snprintf(c, sizeof c, "#%06x", (unsigned)(col & 0xffffff));
    for (; *tpl; tpl++) {
        if (*tpl == '@') {
            memcpy(o, c, 7);
            o += 7;
        } else {
            *o++ = *tpl;
        }
    }
    *o = 0;
    svg_render(svg, size, out);
    free(svg);
}

void icons_init(void)
{
    for (int i = 0; i < IC_COUNT; i++) {
        int px = (i == IC_FOLDER || i == IC_FILE) ? met.icon_big : (i == IC_UP || i == IC_UP_OFF) ? met.icon_up : met.icon;
        render(svg_src[i], px, svg_col[i] ? cfg.c_icon_dim : cfg.c_icon, &icons[i]);
        render(svg_src[i], px, cfg.c_selection_text, &icons_sel[i]);
    }
}

void icon_draw(int id, int x, int y, uint32_t bg, bool on_sel)
{
    SvgBitmap *b = on_sel ? &icons_sel[id] : &icons[id];
    if (!b->rgba)
        return;
    uint32_t stride = (uint32_t)b->w * 4;
    uint8_t *buf = malloc((size_t)stride * (size_t)b->h);
    if (!buf)
        return;
    xcb_image_t *im = xcb_image_create_native(g.c, (uint16_t)b->w, (uint16_t)b->h, XCB_IMAGE_FORMAT_Z_PIXMAP, g.depth,
                                              NULL, stride * (uint32_t)b->h, buf);
    if (!im) {
        free(buf);
        return;
    }
    unsigned br = (bg >> 16) & 255, bgc = (bg >> 8) & 255, bb = bg & 255;
    for (int j = 0; j < b->h; j++) {
        for (int i = 0; i < b->w; i++) {
            const uint8_t *p = b->rgba + ((size_t)j * b->w + i) * 4;
            unsigned a = p[3];
            unsigned r = (p[0] * a + br * (255 - a)) / 255;
            unsigned gg = (p[1] * a + bgc * (255 - a)) / 255;
            unsigned bl = (p[2] * a + bb * (255 - a)) / 255;
            xcb_image_put_pixel(im, (uint32_t)i, (uint32_t)j, (r << 16) | (gg << 8) | bl);
        }
    }
    xcb_image_put(g.c, g.dr, g.gc, im, (int16_t)x, (int16_t)y, 0);
    xcb_image_destroy(im);
    free(buf);
}
