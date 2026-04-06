@echo off
REM ============================================================================
REM Night Market – Windows (MinGW-w64) build
REM
REM Prerequisites:
REM   1) Install MinGW-w64 and put gcc on PATH (e.g. MSYS2: UCRT64 / MINGW64 shell,
REM      and run from that shell, OR add C:\msys64\ucrt64\bin to system PATH).
REM   2) Install raylib built for THE SAME MinGW (same 32/64-bit).
REM   3) set RAYLIB_HOME=C:\path\to\raylib   (must contain include\ and lib\)
REM
REM MSYS2 shortcut (run inside "MSYS2 UCRT64"):
REM   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
REM   Then:  set RAYLIB_HOME=/ucusr/mingw64  (wrong - need actual path)
REM   Raylib in MSYS2: gcc ... $(pkg-config --cflags --libs raylib)
REM   See build_windows_msys2.sh for that one-liner.
REM
REM If the build fails: copy the FULL gcc error (or a screenshot) — "doesn't work"
REM is usually: gcc not on PATH, wrong RAYLIB_HOME, or 32-bit vs 64-bit mismatch.
REM ============================================================================

setlocal
set TARGET=night_market.exe
set SRC_DIR=src
set SRCS=%SRC_DIR%\main.c %SRC_DIR%\game.c %SRC_DIR%\player.c %SRC_DIR%\map.c %SRC_DIR%\dialogue.c %SRC_DIR%\anomaly.c %SRC_DIR%\audio.c %SRC_DIR%\ui.c %SRC_DIR%\minigames\minigames.c %SRC_DIR%\intro_video_stub.c

where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] gcc not found. Add your MinGW bin folder to PATH, or open
    echo         "MSYS2 UCRT64" / "MinGW64" from the Start Menu and run this script there.
    exit /b 1
)

if "%RAYLIB_HOME%"=="" (
    echo [ERROR] RAYLIB_HOME is not set.
    echo Example:  set RAYLIB_HOME=C:\raylib\raylib-5.0_win64_mingw-w64
    exit /b 1
)

if not exist "%RAYLIB_HOME%\include\raylib.h" (
    echo [ERROR] Missing "%RAYLIB_HOME%\include\raylib.h"
    echo Check RAYLIB_HOME points to the raylib root ^(folder that has include\ and lib\^).
    exit /b 1
)

echo Building %TARGET% ...
echo RAYLIB_HOME=%RAYLIB_HOME%

REM No trailing spaces after the ^ line-continuation characters below.
gcc -std=c99 -Wall -Wextra -O2 %SRCS% -o %TARGET% ^
  -I"%RAYLIB_HOME%\include" ^
  -L"%RAYLIB_HOME%\lib" ^
  -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -lm

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Typical fixes:
    echo   - Use 64-bit gcc with 64-bit raylib ^(or both 32-bit^).
    echo   - Ensure libraylib.a or libraylib.dll.a exists in "%RAYLIB_HOME%\lib"
    echo   - From MSYS2, try: build_windows_msys2.sh instead of this .bat
    exit /b 1
)

echo.
echo OK: %TARGET%
echo Run it from this folder so the assets\ directory is next to the .exe .
endlocal
exit /b 0
