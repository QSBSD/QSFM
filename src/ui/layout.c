#include "ui/layout.h"
#include "app.h"
#include "ui/theme.h"

Layout layout_get(void)
{
    Layout l;
    l.ly = TB_H;
    l.lh = g.H - ST_H - l.ly;
    if (l.lh < 0)
        l.lh = 0;
    l.cellw = CELL_W;
    l.cellh = CELL_H;
    l.x0 = SIDE_W + sc(4);
    l.cols = (g.W - l.x0 - SCRL_W) / l.cellw;
    if (l.cols < 1)
        l.cols = 1;
    l.rows = ((int)g.n + l.cols - 1) / l.cols;
    l.vis = l.lh / l.cellh;
    if (l.vis < 1)
        l.vis = 1;
    return l;
}

void layout_clamp_top(const Layout *l)
{
    int mx = l->rows - l->vis;
    if (mx < 0)
        mx = 0;
    if (g.top > mx)
        g.top = mx;
    if (g.top < 0)
        g.top = 0;
}

void layout_thumb(const Layout *l, int *ty, int *th)
{
    int n = l->rows;
    if (n <= l->vis) {
        *ty = l->ly;
        *th = l->lh;
        return;
    }
    int h = l->lh * l->vis / n;
    if (h < sc(24))
        h = sc(24);
    *th = h;
    *ty = l->ly + (l->lh - h) * g.top / (n - l->vis);
}

int layout_hit(const Layout *l, int bx, int by)
{
    if (bx < l->x0 || by < l->ly)
        return -1;
    int col = (bx - l->x0) / l->cellw, row = g.top + (by - l->ly) / l->cellh;
    if (col >= l->cols)
        return -1;
    long idx = (long)row * l->cols + col;
    return idx < (long)g.n ? (int)idx : -1;
}

int layout_search_x(void)
{
    return g.W - SEARCH_W - TB_PAD;
}

int layout_path_w(void)
{
    int w = layout_search_x() - sc(8) - PATH_X;
    return w < sc(60) ? sc(60) : w;
}
