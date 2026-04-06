@echo off
REM ============================================================================
REM Windows MinGW build with STATIC linking (fewer DLL dependencies at runtime)
REM Use this for assignment submission when graders must run night_market.exe
REM without installing MSYS/MinGW DLLs on PATH.
REM
REM Requires: raylib built/installed for MinGW (libraylib.a in RAYLIB_HOME\lib)
REM If linking fails, try the normal build_windows.bat instead and ship DLLs.
REM ============================================================================

set TARGET=night_market.exe

if "%RAYLIB_HOME%"=="" (
    echo Please set RAYLIB_HOME to your raylib installation folder.
    echo Example: set RAYLIB_HOME=C:\raylib
    goto :eof
)

set SRC_DIR=src
set SRCS=%SRC_DIR%\main.c %SRC_DIR%\game.c %SRC_DIR%\player.c %SRC_DIR%\map.c %SRC_DIR%\dialogue.c %SRC_DIR%\anomaly.c %SRC_DIR%\audio.c %SRC_DIR%\ui.c %SRC_DIR%\minigames\minigames.c %SRC_DIR%\intro_video_stub.c

where gcc >nul 2>&1
if errorlevel 1 (
    echo gcc not found. Use MinGW on PATH or build_windows_msys2.sh in MSYS2.
    goto :eof
)
if "%RAYLIB_HOME%"=="" (
    echo Set RAYLIB_HOME to raylib root.
    goto :eof
)

echo Building %TARGET% with -static (may take longer to link) ...

gcc -std=c99 -Wall -Wextra -O2 %SRCS% -o %TARGET% ^
  -I"%RAYLIB_HOME%\include" ^
  -L"%RAYLIB_HOME%\lib" ^
  -static -static-libgcc ^
  -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -lm

if errorlevel 1 (
    echo Static build failed. Try build_windows.bat without -static, or verify raylib MinGW libs.
) else (
    echo Build succeeded. Test night_market.exe on a PC without MinGW in PATH.
    echo Copy night_market.exe + assets\ folder together when zipping.
)
