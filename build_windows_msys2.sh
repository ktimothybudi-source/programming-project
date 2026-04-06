#!/usr/bin/env bash
# Build on Windows using MSYS2 (run this from "MSYS2 MINGW64" or "UCRT64", not cmd.exe).
#
# One-time:  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib
#   (UCRT64 users may use mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib)
#
set -euo pipefail
TARGET="${TARGET:-night_market.exe}"
if ! pkg-config --exists raylib; then
  echo "raylib not found for pkg-config. In MSYS2, install for your environment, e.g.:"
  echo "  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib"
  exit 1
fi
RAY_CFLAGS=$(pkg-config --cflags raylib)
RAY_LIBS=$(pkg-config --libs raylib)
SRCS="src/main.c src/game.c src/player.c src/map.c src/dialogue.c src/anomaly.c src/audio.c src/ui.c src/minigames/minigames.c src/intro_video_stub.c"
gcc -std=c99 -Wall -Wextra -O2 $RAY_CFLAGS $SRCS -o "$TARGET" $RAY_LIBS -lm
echo "OK: $TARGET — run from project root so assets/ sits next to the exe."
