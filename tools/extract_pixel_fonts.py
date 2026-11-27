#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""One-off: reads the two pixel fonts from this machine's font files and writes tools/pixel_font_data.py,
which is what gen_rv_font.py (and the CI build) actually use. Run it only to change the fonts.

  extract_pixel_fonts.py            (needs Pillow, koi6x9.pcf.gz from the X11 cyrillic fonts, unifont.otf)

koi6x9: the X11 "misc-fixed" 6x9 face (public domain). Its glyph table is one slot off in Pillow, hence ord - 1.
Unifont: GNU Unifont 16 px (GPL-2.0-or-later with the font-embedding exception, or SIL OFL-1.1).
"""
import gzip
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, PcfFontFile

OUT = Path(__file__).resolve().parent / "pixel_font_data.py"
PCF = "/usr/share/fonts/X11/cyrillic/koi6x9.pcf.gz"
UNI = "/usr/share/fonts/opentype/unifont/unifont.otf"


def rows(draw_fn, w, h):
    im = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(im)
    d.fontmode = "1"
    draw_fn(d)
    return ["".join("#" if im.getpixel((x, y)) else "." for x in range(w)) for y in range(h)]


def main():
    pcf = PcfFontFile.PcfFontFile(gzip.open(PCF))
    pcf.save("/tmp/koi6x9")
    f6 = ImageFont.load("/tmp/koi6x9.pil")
    fu = ImageFont.truetype(UNI, 16)
    out = ['# SPDX-License-Identifier: GPL-3.0-only',
           '"""Pixel glyphs for ASCII 32..95, made by tools/extract_pixel_fonts.py. Each glyph is rows of "#" (on) and "." (off).',
           'F6X9: X11 misc-fixed 6x9 (public domain), 6 wide, 9 high, capitals on rows 1-6 (baseline below row 6).',
           'UNI16: GNU Unifont, 8 wide, 16 high, capitals on rows 4-13 (baseline below row 13)."""', ""]
    for name, w, h, fn in (("F6X9", 6, 9, lambda c: (lambda d: d.text((0, 0), chr(ord(c) - 1), font=f6, fill=255))),
                           ("UNI16", 8, 16, lambda c: (lambda d: d.text((0, 0), c, font=fu, fill=255)))):
        out.append(f"{name} = {{")
        for code in range(32, 96):
            c = chr(code)
            r = rows(fn(c), w, h)
            out.append(f"    {code}: {r!r},")
        out.append("}\n")
    OUT.write_text("\n".join(out))


main()
