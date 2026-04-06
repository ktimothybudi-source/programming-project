@echo off
REM ============================================================================
REM Night Market – Windows build (MinGW-w64 + raylib)
REM
REM From project root (folder that contains src\ and assets\):
REM   set RAYLIB_HOME=C:\path\to\raylib
REM   build_windows.bat
REM
REM RAYLIB_HOME must contain include\raylib.h and lib\ (e.g. libraylib.a / dll).
REM
REM MSYS2: install matching toolchain + raylib, then set RAYLIB_HOME to env root:
REM   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
REM   Example:  set RAYLIB_HOME=C:\msys64\ucrt64
REM Or run build_windows_msys2.sh inside MSYS2 (uses pkg-config).
REM
REM Static linking (fewer DLLs): see build_windows_static.bat
REM
REM intro_video_stub.c — opening video is macOS-only; Windows build skips it.
REM Run night_market.exe from this folder so assets\ is next to the .exe.
REM ============================================================================

setlocal
set TARGET=night_market.exe
set SRC_DIR=src
REM Same .c set as Makefile (no intro_video_apple.m — stub on Windows).
set SRCS=%SRC_DIR%\main.c %SRC_DIR%\game.c %SRC_DIR%\player.c %SRC_DIR%\map.c %SRC_DIR%\dialogue.c %SRC_DIR%\anomaly.c %SRC_DIR%\audio.c %SRC_DIR%\ui.c %SRC_DIR%\minigames\minigames.c %SRC_DIR%\intro_video_stub.c

where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] gcc not found. Add MinGW bin to PATH, or use MSYS2 UCRT64/MINGW64 shell.
    exit /b 1
)

if "%RAYLIB_HOME%"=="" (
    echo [ERROR] RAYLIB_HOME is not set.
    echo Example:  set RAYLIB_HOME=C:\raylib\raylib-5.0_win64_mingw-w64
    echo MSYS2:     set RAYLIB_HOME=C:\msys64\ucrt64
    exit /b 1
)

if not exist "%RAYLIB_HOME%\include\raylib.h" (
    echo [ERROR] Missing "%RAYLIB_HOME%\include\raylib.h"
    echo Check RAYLIB_HOME points to the raylib root ^(folder with include\ and lib\^).
    exit /b 1
)

echo Building %TARGET% ...
echo RAYLIB_HOME=%RAYLIB_HOME%

gcc -std=c99 -Wall -Wextra -O2 %SRCS% -o %TARGET% ^
  -I"%RAYLIB_HOME%\include" ^
  -L"%RAYLIB_HOME%\lib" ^
  -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -lm

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Typical fixes:
    echo   - 64-bit gcc + 64-bit raylib ^(or both 32-bit^).
    echo   - libraylib.a or libraylib.dll.a exists in "%RAYLIB_HOME%\lib"
    echo   - Try build_windows_msys2.sh from MSYS2, or build_windows_static.bat
    exit /b 1
)

echo.
echo OK: %TARGET%
echo Run from this folder with assets\ beside the .exe.
endlocal
exit /b 0
