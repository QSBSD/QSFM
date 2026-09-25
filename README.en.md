<div align="center">
  <img src="https://avatars.githubusercontent.com/u/329755507?s=200&v=4" alt="QSFM" width="120" height="120">

  # QSFM

  Minimalist file manager

  [![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](./LICENSE)

  [![Русский](https://img.shields.io/badge/lang-Русский-lightgrey)](README.md)
  [![English](https://img.shields.io/badge/lang-English-blue)](README.en.md)
</div>

## About

QSFM is a minimalist file manager combining basic functionality with lightness. Supported platforms: Linux, FreeBSD.

## Features

**File manager configuration**
- **scale** (0.5–10.0) - interface scale (1.0 = 100%)
- **font** - font (fontconfig name or path to .ttf/.otf), monospace
- **font_size** (6–72) - font size at scale=1.0
- **window_width** (300–10000) - window width on startup
- **window_height** (200–10000) - window height on startup
- **sidebar_width** (100–1000) - sidebar width
- **icon_size** (16–256) - file/folder icon size
- **place_icon_size** (8–128) - icon size in the "Places" panel
- **up_icon_size** (8–128) - "up" button icon size
- **cell_width** (48–400) - file cell width in the grid
- **show_hidden** (bool) - show hidden files on startup
- **scroll_lines** (1–50) - grid rows per mouse wheel step
- **date_format** - date format (strftime)
- **start_dir** - startup folder (~ = home)
- **opener** - file opening program
- **terminal** - "Open in terminal" command; empty = prompt
- **color_bg** - file list and input fields background
- **color_toolbar** - top panel and status bar background
- **color_sidebar** - sidebar and dialogs background
- **color_border** - lines and borders
- **color_text** - text
- **color_text_dim** - inactive text, hints
- **color_selection** - selection/menu item/dialog title background
- **color_selection_text** - text and icons on selection
- **color_place_selection** - current place background in the panel
- **color_focus** - active input field border
- **color_button** - buttons background
- **color_menu** - context menu background
- **color_scrollbar** - scrollbar, progress indicator
- **color_scrollbar_thumb** - scrollbar thumb
- **color_icon** - icons
- **color_icon_dim** - inactive icons

Everything is configured in `~/.config/QSFM/config.conf` after the first launch.

## Build / Run

**FreeBSD**
```
sudo pkg install xcb libxcb xcb-image xcb-keysyms xcb-cursor freetype2 fontconfig pkgconf gmake
gmake

./QSFM
```

**Ubuntu/Debian**
```
sudo apt install build-essential pkg-config libxcb1-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-cursor-dev libfreetype-dev libfontconfig1-dev
make

./QSFM
```

## Technologies

- C11, XCB, FreeType2, Fontconfig

## License

Distributed under the terms of the BSD 2-Clause license, as specified
in the [LICENSE](./LICENSE) file.

```
Copyright (c) 2026, QSBSD Contributors
All rights reserved.

Redistribution and use in source and/or binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
