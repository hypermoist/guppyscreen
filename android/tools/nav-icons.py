#!/usr/bin/env python3
"""Writes the nav bar icons as SVGs (android/app/src/main/cpp/icons/nav_*.svg)
from the glyphs main_panel.cpp shows on the other builds (materialdesign_font_40.c),
cut from assets/materialdesignicons-webfont.ttf. Needs fonttools.

    python3 android/tools/nav-icons.py
"""
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
FONT = os.path.join(ROOT, "assets", "materialdesignicons-webfont.ttf")
OUT = os.path.join(ROOT, "android", "app", "src", "main", "cpp", "icons")
GLYPHS = {  # main_panel.cpp's *_SYMBOL
    "nav_home": 0xF02DC,
    "nav_spool": 0xF07DE,
    "nav_console": 0xF018D,
    "nav_tools": 0xF1064,
}

font = TTFont(FONT)
glyphs = font.getGlyphSet()
cmap = font.getBestCmap()
upm = font["head"].unitsPerEm
ascent = font["hhea"].ascent  # the icon's box is the em square, ascent at the top
for name, cp in GLYPHS.items():
    pen = SVGPathPen(glyphs)
    glyphs[cmap[cp]].draw(TransformPen(pen, (1, 0, 0, -1, 0, ascent)))
    with open(os.path.join(OUT, name + ".svg"), "w") as f:
        f.write(f'<!-- Material Design Icons "{cmap[cp]}" (U+{cp:X}), Apache 2.0; '
                f'android/tools/nav-icons.py -->\n'
                f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {upm} {upm}">'
                f'<path fill="#fff" d="{pen.getCommands()}"/></svg>\n')
    print(name)
