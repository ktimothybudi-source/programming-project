 ## BIT Anomalies – 夜市 (Night Market)
 
 **BIT Anomalies – 夜市 (Night Market)** is a small top‑down 2D psychological horror mystery prototype built in C using raylib.
 You explore a haunted Taiwanese‑style night market as a student investigator documenting subtle “anomalies”: shifts in light, space, and memory.
 
 The game is designed as a **vertical slice** / **school project prototype**:
 - Focus on **atmosphere**, **story hooks**, and **interaction systems**
 - Uses **placeholder art** (rectangles, text, glow effects) so it runs without any assets
 - Clean, modular C code that is easy for teachers and classmates to read
 
 ---
 
 ## Controls
 
 - **W / A / S / D** or **Arrow keys**: Move
 - **E**: Interact (talk / inspect)
 - **Space / Enter**: Advance dialogue
 - **Esc**: Pause / unpause
 - **R**: Restart the current run
 - **H**: Toggle on‑screen help overlay
 
 ---
 
 ## Build & Run on macOS
 
 1. Install Homebrew (if you do not already have it) from `https://brew.sh`.
 2. Install raylib:
 
 ```bash
 brew install raylib
 ```
 
 3. From the project root (`/Users/yourname/Desktop/nightmarket` or wherever you cloned it), run:
 
 ```bash
 ./build.sh
 ```
 
 or directly:
 
 ```bash
 make
 ./night_market
 ```
 
 The window opens at 800×600 with a title screen; press **Enter** to begin.
 
 ### Collision mask (required for solid walls)
 
 Place a PNG at **`assets/collision_mask.png`** (same aspect ratio as the playable map, i.e. **2800×3800** design space). **Red** pixels (`R > 200`, `G` and `B` low) become **blocked** collision; everything else is walkable. The image is downsampled (longest side at most 1024px) when building the internal grid. If the file is missing, there is **no** solid collision (you can walk through everything).
 
 Optional: **`assets/map_blueprint.png`** is drawn as the background when present; it does not define collision—only the red mask does.

### Task mask (optional, for pixel-perfect interactable placement)

If present, **`assets/task_mask.png`** overrides interactable positions from a color-coded mask mapped to world space (same image alignment concept as collision). Supported colors:

- Blue `#0000FF`: Badge + Clock Out + Radio (split into 3 stacked bands)
- Green `#00FF00`: Sink
- Yellow `#FFFF00`: Bin
- Purple `#8000FF`: Freezer
- Orange `#FF8000`: Mop
- Pink `#FF00FF`: Generator
- Red `#FF0000`: Lockers (single combined locker interaction)

If `assets/task_mask.png` is missing, the game uses the built-in default task zones.
 
 ---
 
 ## Build & Run on Windows (MinGW + raylib)
 
 1. Install MinGW‑w64 (e.g., via MSYS2).
 2. Install or build raylib for MinGW and note the installation directory.
 3. Set the environment variable `RAYLIB_HOME` to that directory, for example:
 
 ```bat
 set RAYLIB_HOME=C:\raylib
 ```
 
 4. From the project root in a command prompt:
 
 ```bat
 build_windows.bat
 night_market.exe
 ```
 
 If raylib is correctly installed, this produces `night_market.exe` that opens the same prototype.
 
 ---
 
 ## Project Structure
 
 ```text
 src/
   main.c          - Entry point, window and main loop
   game.c / game.h - Game state machine and orchestration
   player.c / player.h   - Player movement, collision, drawing
   map.c / map.h         - Map layout: stalls, walls, lanterns, uncanny alley
   dialogue.c / dialogue.h - Dialogue system and in‑code scripts
   anomaly.c / anomaly.h   - Anomaly triggers, progression, screen flashes
   audio.c / audio.h       - Optional BGM/SFX with safe fallbacks
   ui.c / ui.h             - Title screen, HUD, fades
 assets/
   README_PLACEHOLDER.txt - Notes on adding optional art/audio
 Makefile          - macOS build using clang + Homebrew raylib
 build.sh          - macOS build helper (wraps make)
 build_windows.bat - Windows build helper using MinGW + raylib
 README.md         - This document
 ```
 
 All paths are **relative**, and only standard C + raylib APIs are used, so the same source can be compiled on macOS and Windows.
 
 ---
 
 ## Placeholder Assets & Fallback Rendering
 
 The project is intentionally designed to run **without any real textures or sounds**:
 
 - Visuals use raylib primitives:
   - Player: rounded rectangle with a simple face dot
   - Stalls: colored blocks with labels like “Skewers”, “Masks”, “Incense”
   - Lanterns: glowing circles with color‑shift when anomalies trigger
   - Alleys and uncanny zones: darker areas with subtle vignette
 - Dialogue is embedded directly in code for:
   - A suspicious food vendor
   - A quiet masked customer
   - A child‑like voice from an empty alley
 - Audio manager:
   - Tries to load `assets/audio/night_market_bgm.ogg` and `assets/audio/anomaly_ping.wav`
   - Checks if loading succeeded and **silently skips** playback if files are missing
 
 This makes the project safe to run in any lab where only raylib is installed.
 
 ---
 
 ## Teacher‑Friendly Explanation
 
 This prototype is meant as a **teaching example** of:
 
 - **Modular C design**: separate files for player, map, UI, dialogue, anomalies, audio, and game state.
 - **State machines**: an explicit `GameState` enum for title, intro, exploration, anomaly sequence, and ending.
 - **Interaction systems**:
   - Top‑down collision and movement
   - Interact key (`E`) for stalls and NPCs
   - Visual novel‑style dialogue with speaker names and multiline text
 - **Cross‑platform raylib usage**:
   - No OS‑specific headers or absolute paths
   - Simple Makefile and batch script for macOS and Windows
 - **Robustness**:
   - The game still runs if `assets/` is empty (besides the placeholder readme)
   - Audio loading is optional and guarded
 
 Students can extend the project by:
 
 - Adding real art and sound assets
 - Expanding the night market map and adding more anomalies
 - Implementing branching dialogue, inventory, or save/load
 - Experimenting with lighting, shaders, or additional UI screens
