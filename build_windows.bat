 @echo off
 REM Simple Windows (MinGW-w64) build script for BIT Anomalies – 夜市
 REM Assumes raylib is installed and RAYLIB_HOME points to its root (with include/ and lib/)
 
 set TARGET=night_market.exe
 
 if "%RAYLIB_HOME%"=="" (
     echo Please set RAYLIB_HOME to your raylib installation folder.
     echo For example: set RAYLIB_HOME=C:\raylib
     goto :eof
 )
 
 set SRC_DIR=src
 set SRCS=%SRC_DIR%\main.c %SRC_DIR%\game.c %SRC_DIR%\player.c %SRC_DIR%\map.c %SRC_DIR%\dialogue.c %SRC_DIR%\anomaly.c %SRC_DIR%\audio.c %SRC_DIR%\ui.c
 
 echo Compiling %TARGET% ...
 
 gcc %SRCS% -o %TARGET% ^
  -I%RAYLIB_HOME%\include ^
  -L%RAYLIB_HOME%\lib ^
  -lraylib -lopengl32 -lgdi32 -lwinmm
 
 if errorlevel 1 (
     echo Build failed.
 ) else (
     echo Build succeeded. Run with:
     echo   %TARGET%
 )
