#!/usr/bin/env python3
"""
Split a 6 columns × 2 rows sprite sheet into 12 PNGs.

- Converts near-white background to transparency (RGB sheets).
- Trims empty (fully transparent) margins per cell.
- Row 0 → paul_walk_01 … paul_walk_06 on a shared canvas (bottom-centered) so
  animation does not jitter; game code scales world AABB from texture pixel size.
- Row 1 → npc_young_lady, npc_old_man, npc_teen_boy, npc_old_lady,
          npc_creepy_man, npc_shaman (trim only; sizes may differ).

Usage:
  python3 tools/split_sheet_6x2.py path/to/sheet.png [out_dir]
  python3 tools/split_sheet_6x2.py path/to/sheet.png --no-normalize-walk
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image


def white_to_transparent(im: Image.Image, threshold: int = 18) -> Image.Image:
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    cutoff = 255 - threshold
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if r >= cutoff and g >= cutoff and b >= cutoff:
                px[x, y] = (0, 0, 0, 0)
    return im


def trim_alpha(im: Image.Image) -> Image.Image:
    bbox = im.getbbox()
    if bbox:
        return im.crop(bbox)
    return im


def paste_bottom_center(canvas: Image.Image, im: Image.Image) -> None:
    cw, ch = canvas.size
    w, h = im.size
    x = (cw - w) // 2
    y = ch - h
    canvas.paste(im, (x, y), im)


def normalize_walk_row(cells: list[Image.Image]) -> list[Image.Image]:
    trimmed = [trim_alpha(c) for c in cells]
    max_w = max(im.size[0] for im in trimmed)
    max_h = max(im.size[1] for im in trimmed)
    out: list[Image.Image] = []
    for im in trimmed:
        canvas = Image.new("RGBA", (max_w, max_h), (0, 0, 0, 0))
        paste_bottom_center(canvas, im)
        out.append(canvas)
    return out


def slice_grid(sheet: Image.Image, cols: int, rows: int) -> list[list[Image.Image]]:
    W, H = sheet.size
    grid: list[list[Image.Image]] = []
    for row in range(rows):
        y0 = row * H // rows
        y1 = (row + 1) * H // rows
        row_cells: list[Image.Image] = []
        for col in range(cols):
            x0 = col * W // cols
            x1 = (col + 1) * W // cols
            row_cells.append(sheet.crop((x0, y0, x1, y1)))
        grid.append(row_cells)
    return grid


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sheet", type=Path, help="Source PNG (e.g. 5000×5000 sheet)")
    ap.add_argument(
        "out_dir",
        type=Path,
        nargs="?",
        default=None,
        help="Output directory (default: assets/sprites/characters under repo root)",
    )
    ap.add_argument(
        "-t",
        "--white-threshold",
        type=int,
        default=18,
        help="RGB >= 255-T becomes transparent (default 18)",
    )
    ap.add_argument(
        "--no-normalize-walk",
        action="store_true",
        help="Only trim each walk frame; do not unify canvas (may jitter in-game)",
    )
    args = ap.parse_args()

    root = Path(__file__).resolve().parent.parent
    out_dir = args.out_dir if args.out_dir else root / "assets" / "sprites" / "characters"
    if not args.sheet.exists():
        print(f"Not found: {args.sheet}", file=sys.stderr)
        return 1

    sheet = Image.open(args.sheet).convert("RGB")
    sheet = white_to_transparent(sheet, threshold=args.white_threshold)

    grid = slice_grid(sheet, cols=6, rows=2)
    row_walk = grid[0]
    row_npc = grid[1]

    walk_names = [f"paul_walk_{i:02d}" for i in range(1, 7)]
    npc_names = [
        "npc_young_lady",
        "npc_old_man",
        "npc_teen_boy",
        "npc_old_lady",
        "npc_creepy_man",
        "npc_shaman",
    ]

    if args.no_normalize_walk:
        walk_out = [trim_alpha(im) for im in row_walk]
    else:
        walk_out = normalize_walk_row(row_walk)

    out_dir.mkdir(parents=True, exist_ok=True)

    for name, im in zip(walk_names, walk_out):
        dest = out_dir / f"{name}.png"
        im.save(dest, "PNG")
        print(dest, im.size)

    for name, im in zip(npc_names, row_npc):
        im = trim_alpha(im)
        dest = out_dir / f"{name}.png"
        im.save(dest, "PNG")
        print(dest, im.size)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
