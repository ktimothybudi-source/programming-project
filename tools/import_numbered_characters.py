#!/usr/bin/env python3
"""
Import N-*.png (N = 1..12) from the Cursor assets folder into
assets/sprites/characters with game names. Removes near-black backgrounds,
trims, and normalizes walk frames to one canvas (bottom-centered).

Grid: 1–6 = paul_walk_01..06, 7–12 = NPCs in NpcType order.
Missing indices use fallbacks from fallback_dir if provided.
"""
from __future__ import annotations

import re
import shutil
import sys
from pathlib import Path

from PIL import Image

BLACK_T = 28  # RGB <= this treated as transparent


def key_black(im: Image.Image) -> Image.Image:
    im = im.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if r <= BLACK_T and g <= BLACK_T and b <= BLACK_T:
                px[x, y] = (0, 0, 0, 0)
    return im


def trim(im: Image.Image) -> Image.Image:
    b = im.getbbox()
    return im.crop(b) if b else im


def paste_bottom_center(canvas: Image.Image, im: Image.Image) -> None:
    cw, ch = canvas.size
    w, h = im.size
    canvas.paste(im, ((cw - w) // 2, ch - h), im)


def normalize_walk(frames: list[Image.Image]) -> list[Image.Image]:
    trimmed = [trim(f) for f in frames]
    mw = max(im.size[0] for im in trimmed)
    mh = max(im.size[1] for im in trimmed)
    out = []
    for im in trimmed:
        c = Image.new("RGBA", (mw, mh), (0, 0, 0, 0))
        paste_bottom_center(c, im)
        out.append(c)
    return out


def load_processed(path: Path) -> Image.Image:
    return trim(key_black(Image.open(path)))


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    out_dir = root / "assets" / "sprites" / "characters"
    default_src = Path(
        "/Users/timothykurniawan/.cursor/projects/"
        "Users-timothykurniawan-Desktop-programming-project-main2-with-exe-2-programming-project-main/"
        "assets"
    )
    src_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else default_src
    fb_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else src_dir / "sprites" / "characters"

    by_num: dict[int, Path] = {}
    for p in src_dir.glob("*.png"):
        m = re.match(r"^(\d+)-", p.name)
        if m:
            by_num[int(m.group(1))] = p

    names = ["paul_walk_%02d" % i for i in range(1, 7)] + [
        "npc_young_lady",
        "npc_old_man",
        "npc_teen_boy",
        "npc_old_lady",
        "npc_creepy_man",
        "npc_shaman",
    ]

    walk_processed: list[Image.Image] = []
    for slot in range(1, 7):
        p = by_num.get(slot)
        if p is None:
            fb = fb_dir / f"paul_walk_{slot:02d}.png"
            if not fb.exists():
                print(f"Missing walk slot {slot} and no {fb}", file=sys.stderr)
                return 1
            print(f"paul_walk_{slot:02d} <- fallback {fb.name}")
            walk_processed.append(load_processed(fb))
        else:
            print(f"paul_walk_{slot:02d} <- {p.name}")
            walk_processed.append(load_processed(p))

    walk_out = normalize_walk(walk_processed)
    out_dir.mkdir(parents=True, exist_ok=True)
    for i, im in enumerate(walk_out, start=1):
        (out_dir / f"paul_walk_{i:02d}.png").parent.mkdir(exist_ok=True)
        im.save(out_dir / f"paul_walk_{i:02d}.png", "PNG")

    for slot in range(7, 13):
        key = names[slot - 1]
        p = by_num.get(slot)
        if p is None:
            fb = fb_dir / f"{key}.png"
            if not fb.exists():
                print(f"Missing NPC slot {slot} ({key}) and no {fb}", file=sys.stderr)
                return 1
            print(f"{key} <- fallback {fb.name}")
            im = load_processed(fb)
        else:
            print(f"{key} <- {p.name}")
            im = load_processed(p)
        im.save(out_dir / f"{key}.png", "PNG")

    print("Done ->", out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
