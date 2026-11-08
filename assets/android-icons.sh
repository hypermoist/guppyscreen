#!/bin/bash
# The Android build's resolution-independent icons:
#  - material_svg/*.svg rendered as square PNGs (default 256px) into
#    material_png/, prepared as svg-convert.sh does for the C arrays. The app
#    embeds them and resamples each to the size its screen needs.
#  - materialdesign_nav.ttf, the nav bar glyphs (materialdesign_font_40.c's
#    range) cut from materialdesignicons-webfont.ttf, which the app rasterises
#    at the nav bar's size with LVGL's tiny_ttf.
size="${1:-256}"
cd "$(dirname "$0")" || exit 1
for tool in rsvg-convert pyftsubset; do
    if ! which $tool > /dev/null 2>&1; then
        echo "$tool not installed."
        exit 1
    fi
done
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
rm -rf material_png
mkdir -p material_png
for file in material_svg/*.svg; do
    name=$(basename "$file" .svg)
    rsvg-convert "$file" -w "$size" -h "$size" -f svg -o "$tmp/$name.svg"
    sed -i "s/0%,0%,0%/100%,100%,100%/g" "$tmp/$name.svg"
    rsvg-convert "$tmp/$name.svg" -w "$size" -h "$size" -f png -o "material_png/$name.png"
done
pyftsubset materialdesignicons-webfont.ttf --unicodes=U+F1064,U+F02DC,U+F018D,U+F07DE \
    --no-hinting --desubroutinize --output-file=materialdesign_nav.ttf
if which optipng > /dev/null 2>&1; then
    optipng -quiet -o5 material_png/*.png
fi
