#include "ui/font.h"
#include "app.h"
#include "config.h"
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <xcb/xcb_image.h>

#define OLD_DEFAULT "-misc-fixed-medium-r-normal--*-*-*-*-c-*-iso10646-1"

typedef struct {
    unsigned char *cov; /* w * h coverage, 0..255 */
    int w, h, left, top;
    bool loaded;
} Glyph;

static FT_Library ft;
static FT_Face face;
static Glyph *pages[256]; /* 256 code points per page */

static bool ends_with(const char *s, const char *suf)
{
    size_t n = strlen(s), m = strlen(suf);
    return n >= m && !strcasecmp(s + n - m, suf);
}

static bool looks_like_path(const char *s)
{
    return strchr(s, '/') || ends_with(s, ".ttf") || ends_with(s, ".otf") || ends_with(s, ".ttc") ||
           ends_with(s, ".otc");
}

static bool same(const char *a, const char *b)
{
    return !strcasecmp(a, b);
}

/* resolves a fontconfig name to a file; *exact is set when the family, "family style" or full name matches */
static bool fc_resolve(const char *name, char *file, size_t fn, int *index, bool *exact)
{
    bool ok = false;
    *exact = false;
    if (!FcInit())
        return false;
    FcPattern *pat = FcNameParse((const FcChar8 *)name);
    if (!pat)
        return false;
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult res;
    FcPattern *m = FcFontMatch(NULL, pat, &res);
    if (m) {
        FcChar8 *f;
        if (FcPatternGetString(m, FC_FILE, 0, &f) == FcResultMatch && strlen((char *)f) < fn) {
            snprintf(file, fn, "%s", (char *)f);
            *index = 0;
            FcPatternGetInteger(m, FC_INDEX, 0, index);
            ok = true;
        }
        char req[256];
        snprintf(req, sizeof req, "%s", name);
        char *colon = strchr(req, ':');
        if (colon)
            *colon = 0;
        FcChar8 *fam, *sty, *full;
        for (int i = 0; ok && !*exact && FcPatternGetString(m, FC_FAMILY, i, &fam) == FcResultMatch; i++) {
            if (same(req, (char *)fam))
                *exact = true;
            for (int j = 0; !*exact && FcPatternGetString(m, FC_STYLE, j, &sty) == FcResultMatch; j++) {
                char fs[300];
                snprintf(fs, sizeof fs, "%s %s", (char *)fam, (char *)sty);
                if (same(req, fs))
                    *exact = true;
            }
        }
        for (int i = 0; ok && !*exact && FcPatternGetString(m, FC_FULLNAME, i, &full) == FcResultMatch; i++)
            if (same(req, (char *)full))
                *exact = true;
        FcPatternDestroy(m);
    }
    FcPatternDestroy(pat);
    return ok;
}

static bool open_face(const char *file, int index)
{
    if (face) {
        FT_Done_Face(face);
        face = NULL;
    }
    return FT_New_Face(ft, file, index, &face) == 0;
}

static bool load_named(const char *want)
{
    char file[1024];
    int idx = 0;
    if (looks_like_path(want)) {
        const char *home = getenv("HOME");
        if (want[0] == '~' && (want[1] == '/' || !want[1]) && home)
            snprintf(file, sizeof file, "%s%s", home, want + 1);
        else
            snprintf(file, sizeof file, "%s", want);
        if (access(file, R_OK)) {
            config_warn("Файл шрифта «%s» не найден или недоступен.", file);
            return false;
        }
        if (!open_face(file, 0)) {
            config_warn("Не удалось открыть шрифт «%s».", file);
            return false;
        }
        return true;
    }
    bool exact;
    if (!fc_resolve(want, file, sizeof file, &idx, &exact))
        return false;
    if (!exact && strcmp(want, "monospace")) {
        config_warn("Шрифт «%s» не найден (список: fc-list). Использован monospace.", want);
        return false;
    }
    return open_face(file, idx);
}

bool font_init(void)
{
    if (FT_Init_FreeType(&ft)) {
        config_warn("Не удалось инициализировать FreeType.");
        return false;
    }
    const char *want = cfg.font;
    if (want[0] == '-') { /* old XLFD masks are not supported any more */
        if (strcmp(want, OLD_DEFAULT))
            config_warn("Параметр font: XLFD-маски больше не поддерживаются, использован monospace.");
        want = "monospace";
    }
    if (!load_named(want) && !load_named("monospace")) {
        config_warn("Не найден ни один шрифт (нужен fontconfig и хотя бы один моноширинный шрифт).");
        return false;
    }
    int px = (int)(cfg.font_size * cfg.scale + 0.5);
    if (px < 4)
        px = 4;
    if (px > 1000)
        px = 1000;
    if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)px)) {
        config_warn("Шрифт не масштабируется (нужен TTF/OTF, а не растровый).");
        return false;
    }
    const FT_Size_Metrics *sm = &face->size->metrics;
    g.asc = (int)((sm->ascender + 63) >> 6);
    int desc = (int)((-sm->descender + 63) >> 6);
    g.fh = g.asc + desc;
    if (g.fh < 1)
        g.fh = px;
    g.cw = 0;
    if (FT_Load_Char(face, '0', FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP) == 0)
        g.cw = (int)((face->glyph->advance.x + 32) >> 6);
    if (g.cw < 1)
        g.cw = (int)((sm->max_advance + 32) >> 6);
    if (g.cw < 1)
        g.cw = px / 2 > 0 ? px / 2 : 1;
    return true;
}

static const Glyph *glyph(uint32_t cp)
{
    Glyph **pg = &pages[(cp >> 8) & 255];
    if (!*pg) {
        *pg = calloc(256, sizeof(Glyph));
        if (!*pg)
            return NULL;
    }
    Glyph *gl = &(*pg)[cp & 255];
    if (gl->loaded)
        return gl;
    gl->loaded = true;
    FT_UInt gi = FT_Get_Char_Index(face, cp);
    if (FT_Load_Glyph(face, gi, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP))
        return gl;
    const FT_Bitmap *b = &face->glyph->bitmap;
    if (b->pixel_mode != FT_PIXEL_MODE_GRAY || b->width == 0 || b->rows == 0)
        return gl;
    gl->cov = malloc((size_t)b->width * b->rows);
    if (!gl->cov)
        return gl;
    gl->w = (int)b->width;
    gl->h = (int)b->rows;
    gl->left = face->glyph->bitmap_left;
    gl->top = face->glyph->bitmap_top;
    for (int j = 0; j < gl->h; j++)
        memcpy(gl->cov + (size_t)j * (size_t)gl->w, b->buffer + (long)j * b->pitch, (size_t)gl->w);
    return gl;
}

static uint32_t mix(uint32_t fg, uint32_t old, unsigned a)
{
    unsigned r = (((fg >> 16) & 255) * a + ((old >> 16) & 255) * (255 - a)) / 255;
    unsigned gg = (((fg >> 8) & 255) * a + ((old >> 8) & 255) * (255 - a)) / 255;
    unsigned b = ((fg & 255) * a + (old & 255) * (255 - a)) / 255;
    return (r << 16) | (gg << 8) | b;
}

void font_draw(int x, int y, const xcb_char2b_t *s, int n, uint32_t fg, uint32_t bg)
{
    if (n <= 0 || !face)
        return;
    int w = n * g.cw, h = g.fh;
    uint32_t *pix = malloc((size_t)w * (size_t)h * sizeof *pix);
    if (!pix)
        return;
    for (size_t i = 0; i < (size_t)w * (size_t)h; i++)
        pix[i] = bg;
    for (int i = 0; i < n; i++) {
        const Glyph *gl = glyph((uint32_t)(s[i].byte1 << 8 | s[i].byte2));
        if (!gl || !gl->cov)
            continue;
        int gx = i * g.cw + gl->left, gy = g.asc - gl->top;
        for (int j = 0; j < gl->h; j++) {
            int py = gy + j;
            if (py < 0 || py >= h)
                continue;
            for (int k = 0; k < gl->w; k++) {
                int px = gx + k;
                unsigned a = gl->cov[(size_t)j * (size_t)gl->w + (size_t)k];
                if (px < 0 || px >= w || !a)
                    continue;
                uint32_t *p = &pix[(size_t)py * (size_t)w + (size_t)px];
                *p = mix(fg, *p, a);
            }
        }
    }
    uint32_t stride = (uint32_t)w * 4;
    uint8_t *buf = malloc((size_t)stride * (size_t)h);
    xcb_image_t *im = buf ? xcb_image_create_native(g.c, (uint16_t)w, (uint16_t)h, XCB_IMAGE_FORMAT_Z_PIXMAP, g.depth,
                                                    NULL, stride * (uint32_t)h, buf)
                          : NULL;
    if (im) {
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                xcb_image_put_pixel(im, (uint32_t)i, (uint32_t)j, pix[(size_t)j * (size_t)w + (size_t)i]);
        xcb_image_put(g.c, g.dr, g.gc, im, (int16_t)x, (int16_t)y, 0);
        xcb_image_destroy(im);
    }
    free(buf);
    free(pix);
}
