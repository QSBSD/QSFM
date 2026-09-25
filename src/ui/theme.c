#include "ui/theme.h"
#include "app.h"

Metrics met;

int sc(int v)
{
    return (int)(v * cfg.scale + (v >= 0 ? 0.5 : -0.5));
}

static int mx(int a, int b)
{
    return a > b ? a : b;
}

/* needs g.cw and g.fh */
void metrics_init(void)
{
    met.pad = sc(6);
    met.line = mx(1, sc(1));
    met.icon = sc(cfg.place_icon_size);
    met.icon_up = sc(cfg.up_icon_size);
    met.fld_h = mx(mx(sc(26), g.fh + sc(8)), met.icon_up + sc(4));
    met.tb_h = met.fld_h + 2 * met.pad;
    met.btn = met.fld_h + sc(2);
    met.up_x = sc(6);
    met.path_x = met.up_x + met.btn + sc(10);
    met.search_w = mx(sc(210), 16 * g.cw + sc(8));
    met.side_w = sc(cfg.sidebar_width);
    met.icon_big = sc(cfg.icon_size);
    met.cell_w = mx(mx(sc(cfg.cell_width), 6 * g.cw + 2 * sc(4)), met.icon_big + sc(8));
    met.cell_h = sc(6) + met.icon_big + sc(4) + NAME_LINES * g.fh + sc(6);
    met.st_h = mx(sc(22), g.fh + sc(6));
    met.place_h = mx(sc(26), mx(g.fh + sc(8), met.icon + sc(4)));
    met.place_y0 = met.tb_h + mx(sc(26), g.fh + sc(13));
    met.scrl_w = sc(12);
    met.menu_ih = mx(sc(26), g.fh + sc(8));
    met.menu_pad = sc(3);
    met.menu_w = mx(sc(200), 16 * g.cw + sc(24));
}
