#!/bin/bash
# The Android build's nav bar glyphs: materialdesign_nav.ttf, the four glyphs
# of materialdesign_font_40.c cut from materialdesignicons-webfont.ttf, which
# the app rasterises at the nav bar's size with LVGL's tiny_ttf. (The app's
# other icons are drawn straight from material_svg/.)
cd "$(dirname "$0")" || exit 1
if ! which pyftsubset > /dev/null 2>&1; then
    echo "pyftsubset (fonttools) not installed."
    exit 1
fi
pyftsubset materialdesignicons-webfont.ttf --unicodes=U+F1064,U+F02DC,U+F018D,U+F07DE \
    --no-hinting --desubroutinize --output-file=materialdesign_nav.ttf
