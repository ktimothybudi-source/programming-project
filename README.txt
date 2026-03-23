================================================================================
  BIT ANOMALIES – 夜市 (NIGHT MARKET) / BOHOU SUPERMARKET
  Project housekeeping, build instructions, and code guide
================================================================================

This document explains how to compile, link, and run the game, and how the code
is organized so that even programmers with limited experience can understand it.

ASSIGNMENT SUBMISSION (readme.pdf / readme.doc)
------------------------------------------------
Courses often require a SEPARATE formal document (not only GitHub README.md):
  • readme_DOCUMENTATION.md  — Full design/implementation doc with architecture
    diagrams, data structures, algorithms, AI Usage section, and screenshot
    placeholders. Export to PDF or Word: Save As readme.pdf or readme.doc
  • SUBMISSION_CHECKLIST.txt — What to zip, .exe, video, clean folder

--------------------------------------------------------------------------------
1. WHAT IS THIS PROJECT?
--------------------------------------------------------------------------------

BIT Anomalies is a 2D top-down psychological horror prototype. You play as a
worker in a convenience store (Bohou supermarket) over four days. Each day has
specific tasks (clock in, mop, wash dishes, serve customers, etc.). The store
has a main floor (shelves, cashier), a back area (kitchen, janitor room,
storage), and a basement (generator, lockers). The game uses the C language
and the raylib library for graphics, input, and window management.

--------------------------------------------------------------------------------
2. HOW TO COMPILE AND RUN (STEP BY STEP)
--------------------------------------------------------------------------------

PREREQUISITES
-------------
- A C compiler (on macOS: Xcode Command Line Tools or Homebrew clang)
- The raylib library (https://www.raylib.com/)

INSTALLING RAYLIB ON macOS (using Homebrew)
-------------------------------------------
1. Install Homebrew if you do not have it: open Terminal and go to https://brew.sh
2. In Terminal, run:
      brew install raylib
3. This installs raylib and its headers so the compiler can find them.

COMPILING THE PROJECT (macOS / Linux)
-------------------------------------
1. Open a terminal (Terminal.app on macOS).
2. Go to the project folder. For example, if the project is on your Desktop:
      cd ~/Desktop/nightmarket
   Or wherever you saved the "nightmarket" folder.
3. Run the build script:
      ./build.sh
   Or run make directly:
      make
4. If there are no errors, the compiler produces an executable named:
      night_market
   (On Windows it would be night_market.exe.)

RUNNING THE GAME
----------------
1. Stay in the same folder (the project root) in the terminal.
2. Run:
      ./night_market
3. The game window opens (800x600 pixels). You should see a title screen.
4. Press Enter to start. Use W/A/S/D or arrow keys to move, E to interact.

IMPORTANT: Run the game from the project root (the folder that contains the
"assets" folder). The game looks for "assets/map_blueprint.png" from the
current working directory. If you run from another folder, the map image may
not load (the game will still run with a procedural map).

--------------------------------------------------------------------------------
3. WINDOWS BUILD (OPTIONAL)
--------------------------------------------------------------------------------

If you are on Windows:
1. Install MinGW-w64 (e.g. via MSYS2) and raylib for MinGW.
2. Set the RAYLIB_HOME environment variable to your raylib installation folder.
3. In a command prompt, from the project folder, run:
      build_windows.bat
4. Run night_market.exe from the same folder.

--------------------------------------------------------------------------------
4. EXECUTABLE AND DEMO VIDEO
--------------------------------------------------------------------------------

EXECUTABLE
----------
After you run "make" or "./build.sh", the executable file is:
   night_market   (macOS/Linux)
   night_market.exe (Windows, after build_windows.bat)

You can copy this file together with the "assets" folder to another computer
that has the same operating system and required libraries (e.g. raylib DLL on
Windows). For a standalone demo, you may need to include the raylib runtime
as instructed in raylib's documentation.

VIDEO
-----
To demonstrate the game for a report or submission:
1. Run ./night_market from the project root.
2. Use screen recording software (e.g. macOS: Cmd+Shift+5; Windows: Xbox Game Bar)
   to record a short video (1–2 minutes) showing:
   - Title screen and pressing Enter to start
   - Moving around the store (main area, kitchen, basement)
   - Pressing E near objects to see "Press E to ..." prompts
   - Completing at least one day's tasks (e.g. Day 1: clock in, mop, dishes, garbage, clock out)
3. Save the video (e.g. as demo.mp4) in the project folder or submit it separately
   as required by your assignment.

--------------------------------------------------------------------------------
5. PROJECT FOLDER STRUCTURE (WHAT EACH FILE DOES)
--------------------------------------------------------------------------------

nightmarket/
   Makefile              - Tells "make" how to compile all .c files and link
                           them into the "night_market" executable.
   build.sh              - Script that runs "make" (for macOS/Linux).
   build_windows.bat     - Script that compiles on Windows with MinGW.
   README.md             - Short project overview (Markdown).
   README.txt            - This file: detailed housekeeping and code guide.
   night_market          - The compiled executable (created after "make").

   assets/
      map_blueprint.png  - Optional image used as the in-game map background.
                           If this file exists, the game draws it; otherwise
                           it draws a procedural (code-generated) map.
      README_PLACEHOLDER.txt - Notes about adding more assets.

   src/
      main.c             - Program entry point. Creates the window, initializes
                           audio, creates the game, and runs the main loop
                           (update -> draw) until the window is closed.
      game.c, game.h     - Game state: title, intro, Day 1–4, endings. Handles
                           camera, spawning, interactions (E key), and the
                           "Press E to ..." prompts. Contains the day-by-day
                           task logic.
      map.c, map.h       - Map layout: world size, walls, rooms (main store,
                           kitchen, janitor, storage, basement). Loads the
                           blueprint image if present. Draws background and
                           foreground with Y-sorting so the player can walk
                           behind/in front of objects. Defines all interactables
                           (badge reader, sink, mop, cashier, freezer, stairs,
                           generator, lockers).
      player.c, player.h - Player movement (WASD), collision with walls using
                           a small "feet" collision box, and drawing (simple
                           rectangle with a face dot).
      dialogue.c, dialogue.h - Dialogue system: show text, advance with Enter,
                           different dialogue IDs for each story event.
      anomaly.c, anomaly.h   - Anomaly / horror effects (e.g. screen flashes).
      audio.c, audio.h       - Optional background music and sound effects.
      ui.c, ui.h             - Title screen, HUD (objective text), pause, fade.

--------------------------------------------------------------------------------
6. HOW THE CODE FITS TOGETHER (FOR BEGINNERS)
--------------------------------------------------------------------------------

MAIN LOOP (main.c)
------------------
main() does the following in order:
1. InitWindow(800, 600, ...)  - Opens the game window.
2. InitAudioDevice()         - Prepares for sound (optional).
3. Game_Create(...)          - Allocates the game and creates the map, player,
                               dialogue, etc.
4. while (!WindowShouldClose()) - Loop until the user closes the window:
     - GetFrameTime()         - Time since last frame (for smooth movement).
     - Game_Update(game, dt)  - Update player, camera, interactions, dialogue.
     - BeginDrawing()        - Start drawing this frame.
     - Game_Draw(game)       - Draw map, player, HUD, dialogue, prompts.
     - EndDrawing()          - Show the frame on screen.
5. Game_Destroy(game)        - Free all memory.
6. CloseWindow()             - Close the window.

So: every frame we UPDATE (logic, input, collision) then DRAW (graphics).

GAME STATE (game.c)
-------------------
The game has states: GAME_STATE_TITLE, GAME_STATE_INTRO, GAME_STATE_DAY_1,
GAME_STATE_DAY_2, ... GAME_STATE_ENDING_1. Game_Update() checks the current
state and does the right thing (e.g. in DAY_1, it lets the player move and
press E to interact). When you complete a day's tasks, the game advances to
the next day or state.

MAP (map.c, map.h)
------------------
- WORLD_WIDTH and WORLD_HEIGHT (2800 x 3800) define the size of the play area.
  The camera shows only part of this at a time so one room fills most of the
  screen.
- BuildGeometry() adds rectangles to map->walls[] for every wall, shelf,
  counter, table, etc. The player cannot walk through these.
- BuildInteractables() adds entries to map->interactables[]: each has a
  "bounds" (the object) and a "triggerZone" (where the player stands to press E).
- If "assets/map_blueprint.png" exists, Map_Create() loads it and draws it
  as the background. Otherwise the code draws colored rectangles for floors
  and objects (procedural fallback).
- Map_DrawBackground() and Map_DrawForeground() use the player's Y position
  to draw objects behind or in front of the player (Y-sort).

PLAYER (player.c, player.h)
---------------------------
- Player_GetBounds() returns a rectangle the size of the whole character
  (used for trigger checks and drawing).
- Player_GetCollisionBounds() returns a smaller rectangle: about half the
  width and the lower half of the character ("feet"). This is used for wall
  collision so the player does not get stuck in tight spaces.
- MoveWithCollisions() moves the player, then checks the collision rectangle
  against every wall and pushes the player back if overlapping.

INTERACTION (game.c + map.c)
----------------------------
- When the player presses E, Game_HandleInteractions() checks if the player's
  rectangle overlaps any interactable's triggerZone (or bounds if no trigger
  zone). If yes, it performs the action (e.g. set clockedIn = true, start
  dialogue).
- The "Press E to ..." text is chosen by Game_GetInteractPrompt() and
  Game_GetCashierPrompt() based on the current day and which tasks are done.
  It is drawn in Game_Draw() at the bottom center of the screen when the
  player is in range and not in dialogue.

--------------------------------------------------------------------------------
7. CODE ATTRIBUTION: "Code created by wu deguang"
--------------------------------------------------------------------------------

Parts of this codebase were created or extended by wu deguang. Those sections
are marked in the source files with a comment like:

   // Code created by wu deguang

You will find this comment (and related explanatory comments) in:

- map.c   - Blueprint-based layout (BuildGeometry, BuildInteractables),
            loading and drawing the map image (backgroundTexture),
            Y-sort drawing (rect_center_y, DrawBackground/DrawForeground).
- map.h   - World size (WORLD_WIDTH, WORLD_HEIGHT), Interactable triggerZone,
            backgroundTexture in Map struct.
- game.c  - Camera (Camera2D, zoom, smooth follow, clamp to world bounds),
            spawn positions, Game_GetInteractPrompt, Game_GetCashierPrompt,
            and the on-screen "Press E" prompt drawing.
- player.c - Player_GetCollisionBounds (feet collision),
             MoveWithCollisions using collision bounds.
- player.h - Declaration of Player_GetCollisionBounds.

Reading those comments will help you see exactly which logic was added by
wu deguang and how it works.

--------------------------------------------------------------------------------
8. TROUBLESHOOTING
--------------------------------------------------------------------------------

- "make: command not found"
  Install Xcode Command Line Tools (macOS) or a C compiler (Linux/Windows).

- "raylib.h: No such file or directory"
  Install raylib (e.g. brew install raylib on macOS) and ensure the compiler
  can find it (Makefile uses -I/opt/homebrew/include and -I/usr/local/include).

- "undefined reference to InitWindow" (or other raylib symbols)
  The linker cannot find raylib. Install raylib and ensure -lraylib and the
  correct library path (e.g. -L/opt/homebrew/lib) are in the Makefile.

- Game runs but map is empty or no image
  Run the game from the project root (the folder that contains "assets").
  Check that assets/map_blueprint.png exists if you want the image background.

- Window closes immediately
  Make sure you are not double-clicking the executable from a file manager
  that might change the working directory. Run "./night_market" from the
  project folder in a terminal.

--------------------------------------------------------------------------------
9. SUMMARY
--------------------------------------------------------------------------------

- Compile:  make   (or ./build.sh)
- Run:      ./night_market   (from project root)
- Executable: night_market (or night_market.exe on Windows)
- For a demo: record a short video of gameplay and/or submit the executable
  with the assets folder.
- Code marked "Code created by wu deguang" plus surrounding comments explain
  the blueprint map, camera, interactions, and feet collision.

================================================================================
  End of README.txt
================================================================================
