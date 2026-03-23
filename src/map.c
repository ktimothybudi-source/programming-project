/*
 * ============================================================================
 * map.c — THE STORE LAYOUT (WALLS, ROOMS, INTERACTABLES, DRAWING)
 * ============================================================================
 *
 * COORDINATE SYSTEM:
 *   The map is a big rectangle from (0,0) to (WORLD_WIDTH, WORLD_HEIGHT).
 *   X increases to the RIGHT, Y increases DOWNWARD (same as screen pixels).
 *   The player moves in this same space; the camera shows only part of it.
 *
 * THREE LAYERS OF DATA:
 *   1) walls[] — Rectangles the player CANNOT walk through (collision).
 *   2) interactables[] — Objects you can press E on; each has bounds + triggerZone.
 *   3) Drawing — Either a PNG stretched over the whole world, OR colored rectangles.
 *
 * FUNCTIONS:
 *   BuildGeometry — fills walls[] and sets region rectangles (kitchen, cashier...).
 *   BuildInteractables — fills interactables[] with types and trigger areas.
 *   Map_DrawBackground / Map_DrawForeground — Y-sort: objects above player Y drawn
 *      before the player sprite; objects below drawn after (when not using PNG).
 *
 * Code created by wu deguang is marked below.
 * ============================================================================
 */

#include "map.h"
#include "raylib.h"
#include <stdlib.h>

static void BuildGeometry(Map *map);
static void BuildInteractables(Map *map);
static void BuildNpcs(Map *map);

/* World size constants are in map.h — used for camera clamping in game.c. */
float Map_GetWorldWidth(void)  { return WORLD_WIDTH; }
float Map_GetWorldHeight(void) { return WORLD_HEIGHT; }

Map *Map_Create(int screenWidth, int screenHeight) {
    (void)screenWidth;
    (void)screenHeight;
    Map *m = (Map *)malloc(sizeof(Map));
    if (!m) return NULL;

    m->bounds = (Rectangle){ 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT };
    m->wallCount = 0;
    m->interactableCount = 0;
    m->npcCount = 0;
    m->bloodMoon = false;
    m->backgroundTexture = (Texture2D){ 0 };

    BuildGeometry(m);
    BuildInteractables(m);
    BuildNpcs(m);

    // Code created by wu deguang - load blueprint image as background if present
    if (FileExists("assets/map_blueprint.png")) {
        m->backgroundTexture = LoadTexture("assets/map_blueprint.png");
    }

    return m;
}

void Map_Destroy(Map *map) {
    if (!map) return;
    // Code created by wu deguang - unload blueprint texture if loaded
    if (map->backgroundTexture.id != 0) {
        UnloadTexture(map->backgroundTexture);
    }
    free(map);
}

const Rectangle *Map_GetWalls(const Map *map, int *count) {
    if (count) *count = map->wallCount;
    return map->walls;
}

const Interactable *Map_GetInteractables(const Map *map, int *count) {
    if (count) *count = map->interactableCount;
    return map->interactables;
}

const NpcSpot *Map_GetNpcs(const Map *map, int *count) {
    if (count) *count = map->npcCount;
    return map->npcs;
}

/* Which "room label" contains point (x,y)? Used for basement flag, etc. Order matters (basement first). */
MapRegion Map_GetRegionAt(const Map *map, float x, float y) {
    if (CheckCollisionPointRec((Vector2){ x, y }, map->basementBounds)) return REGION_BASEMENT;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->freezerBounds)) return REGION_FREEZER;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->kitchenBounds)) return REGION_KITCHEN;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->cashierBounds)) return REGION_CASHIER;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->hallwayBounds)) return REGION_HALLWAY;
    return REGION_HALLWAY;
}

// Code created by wu deguang
// Blueprint layout (exact match to attached floor plan, scaled for gameplay):
// Main store: 0..2800 x 0..1200
// Middle: 0..2800 x 1200..2200  [Kitchen 0..1100, Hallway 1100..1400, Janitor 1400..2200 y 1200..1650, Storage 1400..2200 y 1650..2200]
// Basement: 0..2800 x 2200..3800
static void BuildGeometry(Map *map) {
    float W = WORLD_WIDTH;
    float H = WORLD_HEIGHT;

    map->bounds = (Rectangle){ 0, 0, W, H };

    // Region bounds
    map->kitchenBounds   = (Rectangle){ 0, 1200, 1100, 1000 };
    map->hallwayBounds   = (Rectangle){ 1100, 1200, 300, 1000 };
    map->cashierBounds   = (Rectangle){ 2200, 0, 600, 450 };
    map->freezerBounds   = (Rectangle){ 50, 2050, 550, 130 };
    map->basementBounds  = (Rectangle){ 0, 2200, W, 1600 };

    // ----- MAIN STORE (0,0 to 2800, 1200) -----
    map->walls[map->wallCount++] = (Rectangle){ 0, 0, W, 12 };
    map->walls[map->wallCount++] = (Rectangle){ 0, 0, 12, 1200 };
    map->walls[map->wallCount++] = (Rectangle){ W - 12, 0, 12, 1200 };
    map->walls[map->wallCount++] = (Rectangle){ 0, 1188, 1050, 12 };
    map->walls[map->wallCount++] = (Rectangle){ 1150, 1188, 250, 12 };
    map->walls[map->wallCount++] = (Rectangle){ 1400, 1188, W - 1400, 12 };

    // 4 shelves (double-sided footprint)
    map->walls[map->wallCount++] = (Rectangle){ 200, 200, 350, 280 };
    map->walls[map->wallCount++] = (Rectangle){ 2250, 200, 350, 280 };
    map->walls[map->wallCount++] = (Rectangle){ 200, 520, 350, 280 };
    map->walls[map->wallCount++] = (Rectangle){ 2250, 520, 350, 280 };

    // Cashier counter
    map->walls[map->wallCount++] = (Rectangle){ 2350, 80, 420, 220 };

    // ----- MIDDLE SECTION (back area) -----
    // Kitchen left wall (already main store bottom has openings at 1050-1150 and 1250-1350)
    map->walls[map->wallCount++] = (Rectangle){ 0, 1200, 12, 1000 };
    map->walls[map->wallCount++] = (Rectangle){ 1098, 1200, 12, 920 };
    map->walls[map->wallCount++] = (Rectangle){ 1398, 1200, 12, 920 };
    map->walls[map->wallCount++] = (Rectangle){ 0, 2198, 1100, 12 };
    map->walls[map->wallCount++] = (Rectangle){ 1098, 2120, 302, 12 };
    map->walls[map->wallCount++] = (Rectangle){ 1398, 2198, W - 1398, 12 };
    map->walls[map->wallCount++] = (Rectangle){ W - 12, 1200, 12, 1000 };

    // Kitchen: sink/counter row (top wall)
    map->walls[map->wallCount++] = (Rectangle){ 0, 1200, 450, 150 };
    // Kitchen: center table
    map->walls[map->wallCount++] = (Rectangle){ 350, 1650, 400, 350 };
    // Kitchen: garbage bin
    map->walls[map->wallCount++] = (Rectangle){ 120, 1950, 100, 130 };
    // Kitchen: freezer left
    map->walls[map->wallCount++] = (Rectangle){ 50, 2050, 230, 130 };
    // Kitchen: freezer middle
    map->walls[map->wallCount++] = (Rectangle){ 400, 2080, 200, 100 };

    // Janitor: cupboard
    map->walls[map->wallCount++] = (Rectangle){ 1450, 1220, 170, 160 };
    // Janitor: mop bucket area (blocking)
    map->walls[map->wallCount++] = (Rectangle){ 2050, 1580, 130, 60 };

    // Storage: crates
    map->walls[map->wallCount++] = (Rectangle){ 1450, 1680, 300, 470 };
    // Storage: machine
    map->walls[map->wallCount++] = (Rectangle){ 2050, 1700, 130, 450 };

    // ----- BASEMENT -----
    map->walls[map->wallCount++] = (Rectangle){ 0, 2200, 12, 1600 };
    map->walls[map->wallCount++] = (Rectangle){ W - 12, 2200, 12, 1600 };
    map->walls[map->wallCount++] = (Rectangle){ 0, 3798, W, 12 };

    // Generator (left)
    map->walls[map->wallCount++] = (Rectangle){ 200, 2350, 450, 500 };

    // 3 Lockers (right)
    map->walls[map->wallCount++] = (Rectangle){ 2080, 2500, 140, 620 };
    map->walls[map->wallCount++] = (Rectangle){ 2220, 2500, 140, 620 };
    map->walls[map->wallCount++] = (Rectangle){ 2360, 2500, 140, 620 };
}

// Code created by wu deguang - each interactable has bounds (object) and triggerZone (where to stand to press E)
static void BuildInteractables(Map *map) {
    // Badge reader (kitchen, near hallway entrance) - clock in / clock out
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 950, 1280, 100, 60 },
        (Rectangle){ 850, 1300, 110, 80 },
        INTERACT_BADGE,
        "Badge"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 950, 1340, 100, 45 },
        (Rectangle){ 850, 1360, 110, 70 },
        INTERACT_CLOCK_OUT,
        "Clock Out"
    };
    // Sink (kitchen top-left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 80, 1210, 220, 140 },
        (Rectangle){ 80, 1360, 240, 70 },
        INTERACT_SINK,
        "Sink"
    };
    // Garbage (kitchen left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 120, 1950, 100, 130 },
        (Rectangle){ 100, 2090, 140, 55 },
        INTERACT_GARBAGE,
        "Bin"
    };
    // Mop (janitor far right)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 2050, 1580, 130, 60 },
        (Rectangle){ 1920, 1590, 130, 80 },
        INTERACT_MOP,
        "Mop"
    };
    // Radio on cashier counter (main store top-right) - Day 2 turn on radio; customers use cashierBounds in game logic
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 2580, 120, 80, 60 },
        (Rectangle){ 2560, 200, 120, 100 },
        INTERACT_RADIO,
        "Radio"
    };
    // Freezer (kitchen bottom-left, expired meat)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 50, 2050, 230, 130 },
        (Rectangle){ 50, 2190, 230, 65 },
        INTERACT_FREEZER_DOOR,
        "Freezer"
    };
    // Basement stairs down (hallway bottom)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 1100, 2120, 300, 80 },
        (Rectangle){ 1150, 2080, 200, 55 },
        INTERACT_BASEMENT_STAIRS,
        "Stairs"
    };
    // Basement stairs up
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 1100, 2200, 300, 180 },
        (Rectangle){ 1150, 2390, 200, 65 },
        INTERACT_BASEMENT_STAIRS_UP,
        "Stairs Up"
    };
    // Generator (basement left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 200, 2350, 450, 500 },
        (Rectangle){ 200, 2860, 450, 85 },
        INTERACT_GENERATOR,
        "Generator"
    };
    // Lockers (basement right)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 2080, 2500, 140, 620 },
        (Rectangle){ 2080, 3130, 140, 65 },
        INTERACT_LOCKER_1,
        "Locker1"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 2220, 2500, 140, 620 },
        (Rectangle){ 2220, 3130, 140, 65 },
        INTERACT_LOCKER_2,
        "Locker2"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ 2360, 2500, 140, 620 },
        (Rectangle){ 2360, 3130, 140, 65 },
        INTERACT_LOCKER_3,
        "Locker3"
    };
}

/* Placeholder silhouettes at cashier; game logic picks which customer by day when you press E in cashier area. */
static void BuildNpcs(Map *map) {
    float cx = 2550.0f;
    float cy = 350.0f;
    float cw = 80.0f;
    float ch = 100.0f;
    for (int i = 0; i < 6; i++) {
        map->npcs[map->npcCount++] = (NpcSpot){
            (Rectangle){ cx, cy, cw, ch },
            (NpcType)i,
            i == 0 ? "Lady" : i == 1 ? "Old Man" : i == 2 ? "Teen" : i == 3 ? "Old Lady" : i == 4 ? "Creepy" : "Shaman"
        };
    }
}

// Code created by wu deguang - Y-sort: compare object center Y to player Y to draw behind/in front
static inline float rect_center_y(Rectangle r) {
    return r.y + r.height * 0.5f;
}

static void DrawOneShelf(Rectangle r, Color c, Color line) {
    DrawRectangleRec(r, c);
    DrawRectangleLinesEx(r, 1, line);
}

// Code created by wu deguang - draw blueprint image as background when loaded; else procedural + Y-sort
void Map_DrawBackground(const Map *map, float playerY) {
    ClearBackground((Color){ 8, 6, 18, 255 });

    if (map->backgroundTexture.id != 0) {
        // Use blueprint image as full background (stretch to world size)
        Rectangle src = (Rectangle){ 0, 0, (float)map->backgroundTexture.width, (float)map->backgroundTexture.height };
        Rectangle dst = (Rectangle){ 0, 0, WORLD_WIDTH, WORLD_HEIGHT };
        DrawTexturePro(map->backgroundTexture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        // Procedural fallback: floors and Y-sorted objects
        DrawRectangleRec((Rectangle){ 0, 0, WORLD_WIDTH, 1200 }, (Color){ 35, 32, 42, 255 });
        DrawRectangleRec(map->kitchenBounds, (Color){ 28, 30, 40, 255 });
        DrawRectangleLinesEx(map->kitchenBounds, 2, (Color){ 70, 75, 95, 255 });
        DrawRectangleRec(map->hallwayBounds, (Color){ 20, 18, 32, 255 });
        DrawRectangleRec((Rectangle){ 1400, 1200, 800, 450 }, (Color){ 32, 28, 38, 255 });
        DrawRectangleRec((Rectangle){ 1400, 1650, 800, 550 }, (Color){ 30, 28, 40, 255 });
        DrawRectangleLinesEx((Rectangle){ 1400, 1200, 800, 1000 }, 1, (Color){ 65, 60, 85, 255 });
        DrawRectangleRec(map->basementBounds, (Color){ 14, 12, 22, 255 });
        DrawRectangleLinesEx(map->basementBounds, 2, (Color){ 45, 42, 60, 255 });

        Color shelfColor = (Color){ 55, 50, 65, 255 };
        Color shelfLine = (Color){ 90, 85, 110, 255 };
        if (rect_center_y((Rectangle){ 200, 200, 350, 280 }) < playerY) DrawOneShelf((Rectangle){ 200, 200, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2250, 200, 350, 280 }) < playerY) DrawOneShelf((Rectangle){ 2250, 200, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 200, 520, 350, 280 }) < playerY) DrawOneShelf((Rectangle){ 200, 520, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2250, 520, 350, 280 }) < playerY) DrawOneShelf((Rectangle){ 2250, 520, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2350, 80, 420, 220 }) < playerY) {
            DrawRectangleRec((Rectangle){ 2350, 80, 420, 220 }, (Color){ 60, 50, 45, 255 });
            DrawRectangleLinesEx((Rectangle){ 2350, 80, 420, 220 }, 2, (Color){ 100, 90, 80, 255 });
        }
        if (rect_center_y((Rectangle){ 0, 1200, 450, 150 }) < playerY) DrawRectangleRec((Rectangle){ 0, 1200, 450, 150 }, (Color){ 70, 72, 78, 255 });
        if (rect_center_y((Rectangle){ 350, 1650, 400, 350 }) < playerY) DrawRectangleRec((Rectangle){ 350, 1650, 400, 350 }, (Color){ 55, 45, 38, 255 });
        if (rect_center_y((Rectangle){ 120, 1950, 100, 130 }) < playerY) DrawRectangleRec((Rectangle){ 120, 1950, 100, 130 }, (Color){ 40, 38, 42, 255 });
        if (rect_center_y((Rectangle){ 50, 2050, 230, 130 }) < playerY) {
            DrawRectangleRec((Rectangle){ 50, 2050, 230, 130 }, (Color){ 25, 35, 55, 255 });
            DrawRectangleLinesEx((Rectangle){ 50, 2050, 230, 130 }, 1, (Color){ 60, 90, 130, 255 });
        }
        if (rect_center_y((Rectangle){ 400, 2080, 200, 100 }) < playerY) DrawRectangleRec((Rectangle){ 400, 2080, 200, 100 }, (Color){ 30, 38, 52, 255 });
        if (rect_center_y((Rectangle){ 1450, 1220, 170, 160 }) < playerY) DrawRectangleRec((Rectangle){ 1450, 1220, 170, 160 }, (Color){ 50, 42, 38, 255 });
        if (rect_center_y((Rectangle){ 2050, 1580, 130, 60 }) < playerY) DrawRectangleRec((Rectangle){ 2050, 1580, 130, 60 }, (Color){ 45, 55, 70, 255 });
        if (rect_center_y((Rectangle){ 1450, 1680, 300, 470 }) < playerY) DrawRectangleRec((Rectangle){ 1450, 1680, 300, 470 }, (Color){ 48, 40, 35, 255 });
        if (rect_center_y((Rectangle){ 2050, 1700, 130, 450 }) < playerY) DrawRectangleRec((Rectangle){ 2050, 1700, 130, 450 }, (Color){ 35, 38, 42, 255 });
        if (rect_center_y((Rectangle){ 200, 2350, 450, 500 }) < playerY) {
            DrawRectangleRec((Rectangle){ 200, 2350, 450, 500 }, (Color){ 42, 48, 45, 255 });
            DrawRectangleLinesEx((Rectangle){ 200, 2350, 450, 500 }, 1, (Color){ 70, 85, 75, 255 });
        }
        if (rect_center_y((Rectangle){ 2080, 2500, 140, 620 }) < playerY) DrawRectangleRec((Rectangle){ 2080, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2220, 2500, 140, 620 }) < playerY) DrawRectangleRec((Rectangle){ 2220, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2360, 2500, 140, 620 }) < playerY) DrawRectangleRec((Rectangle){ 2360, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2080, 2500, 420, 620 }) < playerY) DrawRectangleLinesEx((Rectangle){ 2080, 2500, 420, 620 }, 1, (Color){ 75, 78, 95, 255 });
        if (rect_center_y((Rectangle){ 1100, 2120, 300, 80 }) < playerY) DrawRectangleRec((Rectangle){ 1100, 2120, 300, 80 }, (Color){ 50, 45, 55, 255 });
        if (rect_center_y((Rectangle){ 1100, 2200, 300, 180 }) < playerY) DrawRectangleRec((Rectangle){ 1100, 2200, 300, 180 }, (Color){ 35, 32, 42, 255 });
    }

    // Interactable labels (behind player only when procedural; when image used, draw all)
    for (int i = 0; i < map->interactableCount; i++) {
        const Interactable *in = &map->interactables[i];
        if (in->label && rect_center_y(in->bounds) < playerY)
            DrawText(in->label, (int)(in->bounds.x + 4), (int)(in->bounds.y - 14), 12, (Color){ 200, 195, 220, 255 });
    }
    for (int i = 0; i < map->npcCount; i++) {
        const NpcSpot *n = &map->npcs[i];
        if (rect_center_y(n->bounds) < playerY) {
            DrawRectangleRounded(n->bounds, 0.25f, 6, (Color){ 18, 14, 28, 255 });
            DrawRectangleLinesEx(n->bounds, 1, (Color){ 85, 80, 105, 255 });
        }
    }
    if (map->bloodMoon) {
        DrawCircle(2720, 80, 40, (Color){ 80, 20, 20, 220 });
        DrawCircle(2720, 80, 28, (Color){ 140, 40, 40, 255 });
    }
}

// Code created by wu deguang - draw objects in front of player (Y-sort); skip when using image
void Map_DrawForeground(const Map *map, float playerY) {
    if (map->backgroundTexture.id == 0) {
        Color shelfColor = (Color){ 55, 50, 65, 255 };
        Color shelfLine = (Color){ 90, 85, 110, 255 };
        if (rect_center_y((Rectangle){ 200, 200, 350, 280 }) >= playerY) DrawOneShelf((Rectangle){ 200, 200, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2250, 200, 350, 280 }) >= playerY) DrawOneShelf((Rectangle){ 2250, 200, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 200, 520, 350, 280 }) >= playerY) DrawOneShelf((Rectangle){ 200, 520, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2250, 520, 350, 280 }) >= playerY) DrawOneShelf((Rectangle){ 2250, 520, 350, 280 }, shelfColor, shelfLine);
        if (rect_center_y((Rectangle){ 2350, 80, 420, 220 }) >= playerY) {
            DrawRectangleRec((Rectangle){ 2350, 80, 420, 220 }, (Color){ 60, 50, 45, 255 });
            DrawRectangleLinesEx((Rectangle){ 2350, 80, 420, 220 }, 2, (Color){ 100, 90, 80, 255 });
        }
        if (rect_center_y((Rectangle){ 0, 1200, 450, 150 }) >= playerY) DrawRectangleRec((Rectangle){ 0, 1200, 450, 150 }, (Color){ 70, 72, 78, 255 });
        if (rect_center_y((Rectangle){ 350, 1650, 400, 350 }) >= playerY) DrawRectangleRec((Rectangle){ 350, 1650, 400, 350 }, (Color){ 55, 45, 38, 255 });
        if (rect_center_y((Rectangle){ 120, 1950, 100, 130 }) >= playerY) DrawRectangleRec((Rectangle){ 120, 1950, 100, 130 }, (Color){ 40, 38, 42, 255 });
        if (rect_center_y((Rectangle){ 50, 2050, 230, 130 }) >= playerY) {
            DrawRectangleRec((Rectangle){ 50, 2050, 230, 130 }, (Color){ 25, 35, 55, 255 });
            DrawRectangleLinesEx((Rectangle){ 50, 2050, 230, 130 }, 1, (Color){ 60, 90, 130, 255 });
        }
        if (rect_center_y((Rectangle){ 400, 2080, 200, 100 }) >= playerY) DrawRectangleRec((Rectangle){ 400, 2080, 200, 100 }, (Color){ 30, 38, 52, 255 });
        if (rect_center_y((Rectangle){ 1450, 1220, 170, 160 }) >= playerY) DrawRectangleRec((Rectangle){ 1450, 1220, 170, 160 }, (Color){ 50, 42, 38, 255 });
        if (rect_center_y((Rectangle){ 2050, 1580, 130, 60 }) >= playerY) DrawRectangleRec((Rectangle){ 2050, 1580, 130, 60 }, (Color){ 45, 55, 70, 255 });
        if (rect_center_y((Rectangle){ 1450, 1680, 300, 470 }) >= playerY) DrawRectangleRec((Rectangle){ 1450, 1680, 300, 470 }, (Color){ 48, 40, 35, 255 });
        if (rect_center_y((Rectangle){ 2050, 1700, 130, 450 }) >= playerY) DrawRectangleRec((Rectangle){ 2050, 1700, 130, 450 }, (Color){ 35, 38, 42, 255 });
        if (rect_center_y((Rectangle){ 200, 2350, 450, 500 }) >= playerY) {
            DrawRectangleRec((Rectangle){ 200, 2350, 450, 500 }, (Color){ 42, 48, 45, 255 });
            DrawRectangleLinesEx((Rectangle){ 200, 2350, 450, 500 }, 1, (Color){ 70, 85, 75, 255 });
        }
        if (rect_center_y((Rectangle){ 2080, 2500, 140, 620 }) >= playerY) DrawRectangleRec((Rectangle){ 2080, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2220, 2500, 140, 620 }) >= playerY) DrawRectangleRec((Rectangle){ 2220, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2360, 2500, 140, 620 }) >= playerY) DrawRectangleRec((Rectangle){ 2360, 2500, 140, 620 }, (Color){ 38, 40, 48, 255 });
        if (rect_center_y((Rectangle){ 2080, 2500, 420, 620 }) >= playerY) DrawRectangleLinesEx((Rectangle){ 2080, 2500, 420, 620 }, 1, (Color){ 75, 78, 95, 255 });
        if (rect_center_y((Rectangle){ 1100, 2120, 300, 80 }) >= playerY) DrawRectangleRec((Rectangle){ 1100, 2120, 300, 80 }, (Color){ 50, 45, 55, 255 });
        if (rect_center_y((Rectangle){ 1100, 2200, 300, 180 }) >= playerY) DrawRectangleRec((Rectangle){ 1100, 2200, 300, 180 }, (Color){ 35, 32, 42, 255 });
    }

    for (int i = 0; i < map->interactableCount; i++) {
        const Interactable *in = &map->interactables[i];
        if (in->label && rect_center_y(in->bounds) >= playerY)
            DrawText(in->label, (int)(in->bounds.x + 4), (int)(in->bounds.y - 14), 12, (Color){ 200, 195, 220, 255 });
    }
    for (int i = 0; i < map->npcCount; i++) {
        const NpcSpot *n = &map->npcs[i];
        if (rect_center_y(n->bounds) >= playerY) {
            DrawRectangleRounded(n->bounds, 0.25f, 6, (Color){ 18, 14, 28, 255 });
            DrawRectangleLinesEx(n->bounds, 1, (Color){ 85, 80, 105, 255 });
        }
    }
}
