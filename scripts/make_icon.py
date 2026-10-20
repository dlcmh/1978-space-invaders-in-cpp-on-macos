#!/usr/bin/env python3
"""Generate the SPACE INVADERS app icon from procedural pixel art.

No arcade sprites, ROM data, fonts, or bitmap assets are used. The alien,
cannon, stars, glows, and 1978 mark are all drawn here, then packaged as an
.icns with Apple's iconutil.
"""
from __future__ import annotations

import math
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
RESOURCES = ROOT / "Resources"
ICONSET = RESOURCES / "AppIcon.iconset"
ICON_PNG = RESOURCES / "AppIcon.png"
ICON_ICNS = RESOURCES / "AppIcon.icns"

SIZE = 2048
OUTPUT = 1024
MARGIN = 96
CORNER = 410

CRAB = (
    "..#.....#..",
    "...#...#...",
    "..#######..",
    ".##.###.##.",
    "###########",
    "#.#######.#",
    "#.#.....#.#",
    "...##.##...",
)

CANNON = (
    "......#......",
    ".....###.....",
    ".....###.....",
    "..#########..",
    ".###########.",
    "#############",
    "#############",
    "##.........##",
)

DIGITS = {
    "1": ("..#", ".##", "..#", "..#", "..#", "..#", "..#"),
    "9": ("###", "#.#", "#.#", "###", "..#", ".##", "##."),
    "7": ("###", "..#", "..#", ".#.", ".#.", ".#.", ".#."),
    "8": ("###", "#.#", "###", "#.#", "#.#", "#.#", "###"),
}


def lerp(a: tuple[int, int, int], b: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def rounded_mask(size: int, margin: int, radius: int) -> Image.Image:
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((margin, margin, size - margin, size - margin), radius=radius, fill=255)
    return mask


def pixel_art(
    image: Image.Image,
    rows: tuple[str, ...],
    origin: tuple[int, int],
    cell: int,
    color: tuple[int, int, int],
) -> None:
    draw = ImageDraw.Draw(image)
    highlight = tuple(min(255, int(channel * 1.35) + 12) for channel in color)
    shadow = tuple(int(channel * 0.55) for channel in color)
    for row, line in enumerate(rows):
        for column, value in enumerate(line):
            if value != "#":
                continue
            x = origin[0] + column * cell
            y = origin[1] + row * cell
            draw.rectangle((x, y, x + cell - 1, y + cell - 1), fill=color + (255,))
            draw.rectangle((x, y, x + cell - 1, y + cell * 0.14), fill=highlight + (255,))
            draw.rectangle((x + cell * 0.86, y, x + cell - 1, y + cell - 1),
                           fill=shadow + (255,))
            draw.rectangle((x, y + cell * 0.86, x + cell - 1, y + cell - 1),
                           fill=shadow + (255,))


def draw_digits(image: Image.Image, text: str, origin: tuple[int, int], cell: int,
                color: tuple[int, int, int]) -> None:
    x = origin[0]
    draw = ImageDraw.Draw(image)
    for character in text:
        glyph = DIGITS[character]
        for row, line in enumerate(glyph):
            for column, value in enumerate(line):
                if value == "#":
                    left = x + column * cell
                    top = origin[1] + row * cell
                    draw.rectangle((left, top, left + cell - 1, top + cell - 1),
                                   fill=color + (255,))
        x += 4 * cell


def glow_from(mask: Image.Image, color: tuple[int, int, int], blur: int) -> Image.Image:
    layer = Image.new("RGBA", mask.size, color + (0,))
    layer.putalpha(mask.point(lambda value: int(value * 0.62)))
    return layer.filter(ImageFilter.GaussianBlur(blur))


def main() -> None:
    RESOURCES.mkdir(exist_ok=True)
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    mask = rounded_mask(SIZE, MARGIN, CORNER)

    background = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(background)
    top = (3, 12, 17)
    bottom = (1, 5, 9)
    for y in range(MARGIN, SIZE - MARGIN):
        t = (y - MARGIN) / max(1, SIZE - 2 * MARGIN)
        bg_draw.line((0, y, SIZE, y), fill=lerp(top, bottom, t) + (255,))
    image.paste(background, (0, 0), mask)

    aura = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    aura_draw = ImageDraw.Draw(aura)
    aura_draw.ellipse((240, 420, 1800, 1720), fill=(22, 170, 89, 80))
    aura_draw.ellipse((1100, 300, 2100, 1200), fill=(190, 30, 110, 32))
    aura = aura.filter(ImageFilter.GaussianBlur(260))
    image.alpha_composite(Image.composite(aura, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)), mask))

    draw = ImageDraw.Draw(image)
    # Fine stars and a few cross glints.
    stars = [
        (330, 360, 5), (520, 230, 3), (1600, 330, 4), (1730, 610, 3),
        (380, 1440, 4), (1580, 1520, 3), (1260, 300, 3), (250, 980, 3),
        (1800, 1050, 3), (690, 1650, 4), (1430, 1760, 3), (980, 220, 2),
    ]
    for x, y, r in stars:
        draw.ellipse((x - r, y - r, x + r, y + r), fill=(122, 255, 178, 150))
    for x, y in ((330, 360), (1600, 330), (380, 1440)):
        draw.line((x - 18, y, x + 18, y), fill=(122, 255, 178, 110), width=2)
        draw.line((x, y - 18, x, y + 18), fill=(122, 255, 178, 110), width=2)

    # Tiny 1978 above the formation, in the same pixel grid language.
    draw_digits(image, "1978", (820, 220), 30, (100, 255, 156))

    # The hero alien, built one pixel at a time.
    crab_cell = 118
    crab_origin = (SIZE // 2 - (11 * crab_cell) // 2, 480)
    crab_mask = Image.new("L", (SIZE, SIZE), 0)
    crab_draw = ImageDraw.Draw(crab_mask)
    for row, line in enumerate(CRAB):
        for column, value in enumerate(line):
            if value == "#":
                x = crab_origin[0] + column * crab_cell
                y = crab_origin[1] + row * crab_cell
                crab_draw.rectangle((x, y, x + crab_cell - 1, y + crab_cell - 1), fill=255)
    crab_glow = glow_from(crab_mask, (46, 255, 114), 60)
    image.alpha_composite(crab_glow)
    pixel_art(image, CRAB, crab_origin, crab_cell, (61, 255, 119))

    # Ground cannon firing a bright centre beam.
    cannon_cell = 42
    cannon_origin = (SIZE // 2 - (13 * cannon_cell) // 2, 1515)
    beam = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    beam_draw = ImageDraw.Draw(beam)
    beam_x = SIZE // 2
    beam_draw.rectangle((beam_x - 15, 1395, beam_x + 15, 1545), fill=(132, 255, 238, 80))
    beam_draw.rectangle((beam_x - 4, 1395, beam_x + 4, 1545), fill=(220, 255, 250, 210))
    beam = beam.filter(ImageFilter.GaussianBlur(8))
    image.alpha_composite(beam)
    cannon_glow_mask = Image.new("L", (SIZE, SIZE), 0)
    cannon_glow_draw = ImageDraw.Draw(cannon_glow_mask)
    for row, line in enumerate(CANNON):
        for column, value in enumerate(line):
            if value == "#":
                x = cannon_origin[0] + column * cannon_cell
                y = cannon_origin[1] + row * cannon_cell
                cannon_glow_draw.rectangle((x, y, x + cannon_cell - 1, y + cannon_cell - 1),
                                           fill=255)
    cannon_glow = glow_from(cannon_glow_mask, (52, 238, 255), 34)
    image.alpha_composite(cannon_glow)
    pixel_art(image, CANNON, cannon_origin, cannon_cell, (194, 255, 246))

    # A subtle cabinet-like vignette and scanlines preserve the CRT mood.
    scan = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    scan_draw = ImageDraw.Draw(scan)
    for y in range(0, SIZE, 8):
        scan_draw.line((0, y, SIZE, y), fill=(0, 0, 0, 26))
    scan = Image.composite(scan, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)), mask)
    image.alpha_composite(scan)

    vignette = Image.new("L", (SIZE, SIZE), 0)
    vignette_draw = ImageDraw.Draw(vignette)
    vignette_draw.ellipse((-SIZE * 0.10, -SIZE * 0.10, SIZE * 1.10, SIZE * 1.10), fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(220))
    darkness = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 120))
    darkness.putalpha(Image.eval(vignette, lambda value: 255 - value))
    image.alpha_composite(Image.composite(darkness, Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0)),
                                          mask))

    final_alpha = image.getchannel("A")
    final_alpha = Image.composite(final_alpha, Image.new("L", (SIZE, SIZE), 0), mask)
    image.putalpha(final_alpha)
    image = image.resize((OUTPUT, OUTPUT), Image.Resampling.LANCZOS)
    image.save(ICON_PNG)

    ICONSET.mkdir(exist_ok=True)
    for size in (16, 32, 128, 256, 512):
        image.resize((size, size), Image.Resampling.LANCZOS).save(
            ICONSET / f"icon_{size}x{size}.png")
        image.resize((size * 2, size * 2), Image.Resampling.LANCZOS).save(
            ICONSET / f"icon_{size}x{size}@2x.png")
    subprocess.run(["iconutil", "-c", "icns", str(ICONSET), "-o", str(ICON_ICNS)], check=True)
    print(f"Wrote {ICON_PNG.relative_to(ROOT)} and {ICON_ICNS.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
