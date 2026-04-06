@echo off
REM ============================================================================
REM Night Market – Windows MinGW STATIC link (fewer runtime DLLs)
REM Use for distribution when graders run night_market.exe without MSYS on PATH.
REM Requires static libraylib.a in RAYLIB_HOME\lib — see build_windows.bat for setup.
REM ============================================================================

setlocal
set TARGET=night_market.exe
set SRC_DIR=src
set SRCS=%SRC_DIR%\main.c %SRC_DIR%\game.c %SRC_DIR%\player.c %SRC_DIR%\map.c %SRC_DIR%\dialogue.c %SRC_DIR%\anomaly.c %SRC_DIR%\audio.c %SRC_DIR%\ui.c %SRC_DIR%\minigames\minigames.c %SRC_DIR%\intro_video_stub.c

if "%RAYLIB_HOME%"=="" (
    echo [ERROR] Set RAYLIB_HOME to raylib root ^(include + lib with libraylib.a^).
    echo Example: set RAYLIB_HOME=C:\msys64\ucrt64
    exit /b 1
)

where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] gcc not on PATH. Use MinGW or MSYS2 UCRT64/MINGW64.
    exit /b 1
)

echo Building %TARGET% ^(-static^) ...

gcc -std=c99 -Wall -Wextra -O2 %SRCS% -o %TARGET% ^
  -I"%RAYLIB_HOME%\include" ^
  -L"%RAYLIB_HOME%\lib" ^
  -static -static-libgcc ^
  -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -lm

if errorlevel 1 (
    echo [ERROR] Static link failed. Try build_windows.bat without -static / ship raylib DLLs.
    exit /b 1
)

echo OK: %TARGET% — test on a PC without MinGW in PATH; zip exe + assets\
endlocal
exit /b 0
