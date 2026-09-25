#ifndef FM_LAYOUT_H
#define FM_LAYOUT_H

typedef struct {
    int ly, lh, vis, x0, cols, rows, cellw, cellh;
} Layout;

Layout layout_get(void);
void layout_clamp_top(const Layout *l);
void layout_thumb(const Layout *l, int *ty, int *th);
int layout_search_x(void);
int layout_path_w(void);
/* index of the entry under the point, or -1 */
int layout_hit(const Layout *l, int bx, int by);
#endif
