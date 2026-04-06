#!/usr/bin/env python3
"""Turn white (and near-white) pixels transparent in PNGs."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image


def white_to_transparent(im: Image.Image, threshold: int = 18) -> Image.Image:
    """Pixels with R,G,B all >= (255 - threshold) become fully transparent."""
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


def process_file(
    path: Path,
    *,
    threshold: int,
    trim: bool,
    inplace: bool,
    suffix: str,
) -> Path | None:
    try:
        im = Image.open(path)
    except OSError as e:
        print(f"Skip (open error): {path}: {e}", file=sys.stderr)
        return None
    out = white_to_transparent(im, threshold=threshold)
    if trim:
        out = trim_alpha(out)
    dest = path if inplace else path.with_name(f"{path.stem}{suffix}{path.suffix}")
    out.save(dest, "PNG")
    return dest


def collect_inputs(paths: list[Path], recursive: bool) -> list[Path]:
    files: list[Path] = []
    for p in paths:
        if p.is_file() and p.suffix.lower() in {".png", ".webp"}:
            files.append(p)
        elif p.is_dir():
            pattern = "**/*" if recursive else "*"
            for child in sorted(p.glob(pattern)):
                if(child.is_file()
                    and child.suffix.lower() in {".png", ".webp"}):
                    files.append(child)
        else:
            print(f"Not found: {p}", file=sys.stderr)
    seen: set[Path] = set()
    unique: list[Path] = []
    for f in files:
        rp = f.resolve()
        if rp not in seen:
            seen.add(rp)
            unique.append(f)
    return unique


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "paths",
        nargs="+",
        type=Path,
        help="PNG/WebP files or directories",
    )
    ap.add_argument(
        "-t",
        "--threshold",
        type=int,
        default=18,
        help="Treat RGB >= 255-T as background (default 18)",
    )
    ap.add_argument("--trim", action="store_true", help="Crop to non-transparent bbox")
    ap.add_argument(
        "--in-place",
        action="store_true",
        help="Overwrite input files (default: write stem+suffix+.png)",
    )
    ap.add_argument(
        "--suffix",
        default="_nobg",
        help="Output suffix when not --in-place (default _nobg)",
    )
    ap.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="When path is a directory, include subfolders",
    )
    args = ap.parse_args()
    files = collect_inputs(args.paths, args.recursive)
    if not files:
        print("No matching images.", file=sys.stderr)
        return 1
    n = 0
    for f in files:
        dest = process_file(
            f,
            threshold=args.threshold,
            trim=args.trim,
            inplace=args.in_place,
            suffix=args.suffix,
        )
        if dest:
            print(dest, Image.open(dest).size)
            n += 1
    print(f"Done: {n} file(s)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
