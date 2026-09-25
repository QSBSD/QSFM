CC ?= cc
PKGS = xcb xcb-image xcb-keysyms xcb-cursor freetype2 fontconfig
CFLAGS ?= -O2
CFLAGS += -std=gnu11 -Wall -Wextra -Wno-format-truncation -MMD -MP -Isrc -Ithird_party $(shell pkg-config --cflags $(PKGS))
LDLIBS = $(shell pkg-config --libs $(PKGS)) -lpthread -lm
SRC = \
	src/app.c \
	src/config.c \
	src/fs/copy.c \
	src/fs/delete.c \
	src/fs/launch.c \
	src/fs/listing.c \
	src/fs/mount.c \
	src/fs/opctx.c \
	src/fs/places.c \
	src/fs/selection.c \
	src/main.c \
	src/svg/svgrender.c \
	src/ui/actions.c \
	src/ui/clipboard.c \
	src/ui/dialog.c \
	src/ui/font.c \
	src/ui/gfx.c \
	src/ui/icons.c \
	src/ui/keyboard.c \
	src/ui/layout.c \
	src/ui/menu.c \
	src/ui/mouse.c \
	src/ui/nav.c \
	src/ui/theme.c \
	src/ui/ti_view.c \
	src/ui/ui.c \
	src/ui/view.c \
	src/util/fmt.c \
	src/util/keysym.c \
	src/util/path.c \
	src/util/textinput.c \
	src/util/utf8.c \
	src/x11.c
OBJ = $(SRC:.c=.o)

QSFM: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f QSFM $(OBJ) $(OBJ:.o=.d)

-include $(OBJ:.o=.d)

.PHONY: clean
