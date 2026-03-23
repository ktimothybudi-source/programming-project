# BIT Anomalies – 夜市 (Night Market)  
## Technical documentation for assignment submission  
**Document type:** Use this file as the source for **readme.pdf** or **readme.doc**  
**How to submit as PDF:** Open in **Microsoft Word** (File → Open → select this file, or paste content), or **Google Docs**, then **Save As PDF**.  
Alternatively: install [Pandoc](https://pandoc.org/) and run:  
`pandoc readme_DOCUMENTATION.md -o readme.pdf --pdf-engine=xelatex`  
(or print from VS Code Markdown preview → **Print → Save as PDF**).

---

## Table of contents
1. [Project overview](#1-project-overview)  
2. [Design goals](#2-design-goals)  
3. [Architecture (how modules work together)](#3-architecture-how-modules-work-together)  
4. [Key data structures](#4-key-data-structures)  
5. [Core algorithms and flow](#5-core-algorithms-and-flow)  
6. [Implementation notes](#6-implementation-notes)  
7. [Building and running](#7-building-and-running)  
8. [AI usage](#8-ai-usage)  
9. [Appendix: diagram index](#9-appendix-diagram-index)  

---

## 1. Project overview

**BIT Anomalies – 夜市** is a **2D top-down** horror-mystery prototype written in **C** using **raylib**. The player works in a **convenience store (Bohou supermarket)** over **four in-game days**, completing tasks (clock in/out, mopping, dishes, serving customers, basement events, etc.).

**Technology stack:** C99, raylib (graphics, input, window, optional audio).

**Repository layout (for reference):**
```
nightmarket/
  src/          — C source and headers
  assets/       — map_blueprint.png (optional background)
  Makefile      — macOS/Linux build
  build_windows.bat — Windows (MinGW) build
  readme_DOCUMENTATION.md — this document
```

---

## 2. Design goals

| Goal | Approach |
|------|----------|
| Readable, modular C | Separate files: `game`, `map`, `player`, `dialogue`, `ui`, `anomaly`, `audio` |
| Clear progression | `GameState` enum + day/task flags on `Game` |
| Explorable map | Large world coordinates; camera follows player; walls from rectangles |
| Interactions | `Interactable` with **bounds** (object) and **triggerZone** (where to press E) |
| Optional art | Procedural rectangles **or** stretched blueprint PNG as background |

---

## 3. Architecture (how modules work together)

### 3.1 High-level diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        main.c                                │
│  InitWindow → Game_Create → loop { Game_Update, Game_Draw } │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     game.c / game.h                          │
│  • GameState (title, intro, day1–4, endings)                 │
│  • Task flags (clockedIn, moppingDone, …)                    │
│  • Camera2D (follow player, clamp to world)                  │
│  • Input: E interactions, cashier area, objectives             │
└───────┬─────────────────┬─────────────────┬─────────────────┘
        │                 │                 │
        ▼                 ▼                 ▼
   map.c/h          player.c/h       dialogue.c/h
   walls,           movement,         lines, advance,
   interactables,   collision         Dialogue_Start
   regions, draw
```

### 3.2 Header vs source files

| Role | Examples |
|------|----------|
| **`.h` (headers)** | Declares `struct Game`, `enum GameState`, function prototypes. Lets many `.c` files share one contract without duplicating code. |
| **`.c` (sources)** | Implements functions. Only one `.c` file should define each global function (the linker merges them). |

**Include order:** `main.c` includes `game.h`; `game.c` includes `map.h`, `player.h`, `dialogue.h`, etc. Forward declarations in `game.h` avoid circular includes between headers.

### 3.3 Communication between parts

- **`Game`** holds pointers: `Map *`, `Player *`, `DialogueSystem *`, … — the game loop always goes through **`Game_Update` / `Game_Draw`**.
- **Map** stores **geometry** and **interactables**; it does **not** encode story logic. **`Game_HandleInteractions`** reads `InteractableType` and updates day/task flags or starts dialogue.
- **Dialogue** can start from interaction or timers; **`Dialogue_IsActive`** may block movement.

---

## 4. Key data structures

### 4.1 `Game` (see `game.h`)

Central **aggregate** for one play session:

- **`GameState state`** — current mode (title, intro, per-day play, scripted sequences, ending).
- **`int currentDay`** — 1–4.
- **Booleans** — e.g. `clockedIn`, `moppingDone`, `day2YoungLadyServed`, `generatorFixed`, …  
  **Why:** Simple flags are easy to debug and map directly to assignment story beats.
- **`Camera2D camera`** — top-down follow; `offset` = screen center, `target` = world point to look at.
- **Pointers** to subsystems — single ownership from `Game_Create` / `Game_Destroy`.

### 4.2 `Map` (see `map.h`)

- **`Rectangle walls[MAX_WALLS]`** — axis-aligned obstacles for collision.
- **`Interactable interactables[MAX_INTERACTABLES]`** — gameplay hotspots.
- **Region rectangles** — `kitchenBounds`, `hallwayBounds`, `cashierBounds`, `basementBounds`, … for “where is the player?” checks.
- **`Texture2D backgroundTexture`** — optional; if loaded, full-world background image.

### 4.3 `Interactable`

```text
bounds       — Footprint of the object (and blocking area where applicable)
triggerZone  — Rectangle where the player should stand to press E
type         — InteractableType (badge, sink, mop, radio, stairs, generator, lockers, …)
label        — Short text drawn near the object (debug/UX)
```

**Why two rectangles?** The object may be wide; the **interaction** should happen in front or beside it, not on top of solid collision.

### 4.4 `Player`

- **`Vector2 position`** — Character center (drawing uses full `size`; collision can use a smaller “feet” box — see `Player_GetCollisionBounds` in `player.c`).

---

## 5. Core algorithms and flow

### 5.1 Main loop (every frame)

```
┌──────────────┐
│  Poll input  │
└──────┬───────┘
       ▼
┌──────────────────┐
│  Game_Update(dt) │  — state machine, movement if allowed, interactions
└──────┬───────────┘
       ▼
┌──────────────────┐
│  Game_Draw()     │  — BeginMode2D(camera) → map → player → HUD → dialogue
└──────────────────┘
```

### 5.2 State and day progression (simplified)

```
TITLE ──Enter──► INTRO ──► DAY_1 ──tasks done──► DAY_2 ──► … ──► ENDING
                      │         │                      │
                      │         └── dialogue / flags   └── advance when conditions met
                      └── dialogue closes → DAY_1
```

Actual states are listed in **`GameState`** (`game.h`). Transitions are implemented in **`Game_Update`** and helper functions (e.g. `Game_ChangeState`, `Game_AdvanceDayIfReady`).

### 5.3 Interaction algorithm (E key)

1. If dialogue is active, **E** may be ignored or handled by dialogue first (implementation-specific).
2. Build **`Rectangle`** for the player (full body for overlap with **trigger zones**).
3. For each **`Interactable`**, use **`triggerZone`** if non-zero; else **`bounds`**.
4. On first overlap, **`switch (type)`** — set flags, call **`Dialogue_Start`**, teleport for stairs, etc.
5. If no interactable hit, test overlap with **`cashierBounds`** for customer/shaman sequences.

### 5.4 Camera follow

- **`camera.target`** moves toward **`player.position`** with smoothing (lerp).
- **Clamp** `target` so half the visible world rectangle never crosses **`[0, WORLD_WIDTH]` × `[0, WORLD_HEIGHT]`**.

### 5.5 Map drawing (optional Y-sort)

When **no** full-screen texture: objects with **center Y < player Y** draw in **background** pass; others in **foreground** pass, so the player can appear **behind** or **in front** of props. When the **blueprint texture** is used, the image is one layer; labels/NPCs still follow the same ordering rules where applicable.

---

## 6. Implementation notes

- **World size:** `WORLD_WIDTH` × `WORLD_HEIGHT` in `map.h` — camera shows a **window** into this larger space (not the whole map at once).
- **Assets:** `assets/map_blueprint.png` is optional; if missing, procedural colored rectangles draw the layout.
- **Comments in code:** Functions and non-trivial blocks are commented; some sections are marked for coursework attribution where applicable.

---

## 7. Building and running

### 7.1 macOS / Linux

```bash
brew install raylib   # macOS example
cd nightmarket
make
./night_market
```

Run from the **project root** so **`assets/`** resolves.

### 7.2 Windows — executable for submission

1. Install **MinGW-w64** and a **static** or standard **raylib** build for MinGW.  
2. Set **`RAYLIB_HOME`** to the raylib folder (contains `include/`, `lib/`).  
3. Run **`build_windows.bat`**.  
4. Output: **`night_market.exe`**.

**Reducing DLL dependency:** The batch file may use **static linking** flags (see comments in `build_windows.bat`). If the grader’s PC lacks MSYS DLLs, prefer building with **`-static`** where compatible with your raylib library, or ship **all** required `.dll` files next to the `.exe` in the zip (check MinGW and raylib docs for your toolchain).

**Test before submitting:** Run **`night_market.exe`** on a **clean Windows machine** or VM without your dev tools installed.

### 7.3 Demo video (required)

Record a **short screen capture** (with **narration** preferred):

- Title screen → start game  
- Walk through **main floor**, **kitchen**, **basement**  
- Show **Press E** prompts and **complete at least one day’s tasks**  
- End with the strongest feature (e.g. Day 4 atmosphere, blackout, or basement)

---

## 8. AI usage

This section summarizes how **AI-assisted tools** were used during development, per course requirements.

| Area | How AI was used | Human oversight |
|------|-----------------|-----------------|
| **Planning / structure** | Suggestions for module boundaries (game vs map vs player), task flow by day | Team decided final story beats and scope |
| **Code** | Drafting and refactoring C (raylib API usage, camera, collisions, interactable triggers, optional background image loading) | All code was reviewed, compiled, and tested locally; assignments and rubric followed |
| **Documentation** | Drafting README-style explanations, diagram ideas, comment templates | Facts verified against actual files; screenshots and video produced by the team |
| **Debugging** | Ideas for linker paths, include order, collision math | Final fixes validated by building and running the game |

**Principle:** AI outputs were treated as **drafts**. The team remains responsible for **correctness**, **originality where required by the instructor**, and **honesty** in this disclosure.

---

## 9. Appendix: diagram index

| Figure | Description |
|--------|-------------|
| §3.1 | Module dependency diagram (main → game → map/player/dialogue) |
| §5.1 | Per-frame update/draw loop |
| §5.2 | Simplified state/day flow |

### Screenshot placeholders (paste images when exporting to PDF/Word)

**[Figure A – Title screen]**  
*Replace with screenshot: game window on title.*

**[Figure B – Gameplay – kitchen / store]**  
*Replace with screenshot: player moving in store.*

**[Figure C – Interaction prompt]**  
*Replace with screenshot: “Press E to …” visible.*

**[Figure D – Dialogue]**  
*Replace with screenshot: dialogue box if applicable.*

---

**End of readme_DOCUMENTATION.md**
