#!/usr/bin/env python3
"""Slice the 1024-wide two-row character sheet into individual PNGs."""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image


def black_to_transparent(im: Image.Image, threshold: int = 12) -> Image.Image:
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if r <= threshold and g <= threshold and b <= threshold:
                px[x, y] = (0, 0, 0, 0)
    return im


def trim_alpha(im: Image.Image) -> Image.Image:
    bbox = im.getbbox()
    if bbox:
        return im.crop(bbox)
    return im


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    default_src = root / "assets" / "Pixel_art_character_animations_grid__1_-6e2b5972-9ee9-461a-90a3-455536ba58a1.png"
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else default_src
    out_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else root / "assets" / "sprites" / "characters"

    if not src.exists():
        print(f"Source not found: {src}", file=sys.stderr)
        return 1

    sheet = Image.open(src).convert("RGB")
    W, H = sheet.size
    mid = H // 2

    top_names = [
        "paul_walk_01",
        "paul_walk_02",
        "paul_walk_03",
        "paul_walk_04",
        "paul_walk_05",
        "paul_walk_06",
        "npc_young_lady",
    ]
    bot_names = [
        "npc_old_man",
        "npc_teen_boy",
        "npc_old_lady",
        "npc_creepy_man",
        "npc_shaman",
        "npc_mike_hawk",
    ]

    out_dir.mkdir(parents=True, exist_ok=True)

    def slice_row(y0: int, y1: int, cols: int, names: list[str]) -> None:
        for i, name in enumerate(names):
            x0 = i * W // cols
            x1 = (i + 1) * W // cols
            cell = sheet.crop((x0, y0, x1, y1))
            cell = black_to_transparent(cell)
            cell = trim_alpha(cell)
            dest = out_dir / f"{name}.png"
            cell.save(dest, "PNG")
            print(dest, cell.size)

    slice_row(0, mid, 7, top_names)
    slice_row(mid, H, 6, bot_names)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
