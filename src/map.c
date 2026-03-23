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
 * DATA:
 *   1) collisionGrid — from assets/collision_mask.png (red pixels = blocked).
 *   2) interactables[] — Objects you can press E on; each has bounds + triggerZone.
 *   3) Drawing — PNG stretched over the world, OR procedural colored rectangles.
 *
 * FUNCTIONS:
 *   BuildGeometry — sets region rectangles (kitchen, cashier...) only.
 *   BuildInteractables — fills interactables[] with types and trigger areas.
 *   Map_DrawBackground / Map_DrawForeground — Y-sort: objects above player Y drawn
 *      before the player sprite; objects below drawn after (when not using PNG).
 *
 * Code created by wu deguang is marked below.
 * ============================================================================
 */

#include "map.h"
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

/* Design-time coordinates × WORLD_SCALE — keeps geometry in sync with WORLD_WIDTH/HEIGHT. */
#define M(x) ((float)(x) * WORLD_SCALE)

/* Static obstacle rectangles — shared by BuildGeometry (walls) and procedural drawing. */
static const Rectangle OB_SHELF_NW   = { M(200), M(200), M(350), M(280) };
static const Rectangle OB_SHELF_NE   = { M(2250), M(200), M(350), M(280) };
static const Rectangle OB_SHELF_SW   = { M(200), M(520), M(350), M(280) };
static const Rectangle OB_SHELF_SE   = { M(2250), M(520), M(350), M(280) };
static const Rectangle OB_COUNTER    = { M(2350), M(80), M(420), M(220) };
static const Rectangle OB_SINK_ROW   = { 0, M(1200), M(450), M(150) };
static const Rectangle OB_TABLE      = { M(350), M(1650), M(400), M(350) };
static const Rectangle OB_BIN        = { M(120), M(1950), M(100), M(130) };
static const Rectangle OB_FREEZER_L  = { M(50), M(2050), M(230), M(130) };
static const Rectangle OB_FREEZER_M  = { M(400), M(2080), M(200), M(100) };
static const Rectangle OB_JANITOR_CUP = { M(1450), M(1220), M(170), M(160) };
static const Rectangle OB_MOP        = { M(2050), M(1580), M(130), M(60) };
static const Rectangle OB_STORAGE_CRATES = { M(1450), M(1680), M(300), M(470) };
static const Rectangle OB_STORAGE_MACHINE = { M(2050), M(1700), M(130), M(450) };
static const Rectangle OB_GENERATOR  = { M(200), M(2350), M(450), M(500) };
static const Rectangle OB_LOCKER_1   = { M(2080), M(2500), M(140), M(620) };
static const Rectangle OB_LOCKER_2   = { M(2220), M(2500), M(140), M(620) };
static const Rectangle OB_LOCKER_3   = { M(2360), M(2500), M(140), M(620) };
static const Rectangle OB_LOCKERS_BBOX = { M(2080), M(2500), M(420), M(620) };
static const Rectangle OB_STAIRS_DOWN = { M(1100), M(2120), M(300), M(80) };
static const Rectangle OB_STAIRS_UP   = { M(1100), M(2200), M(300), M(180) };

/* Floor-only regions (procedural art). */
static const Rectangle OB_FLOOR_MAIN = { 0, 0, WORLD_WIDTH, M(1200) };
static const Rectangle OB_FLOOR_JANITOR = { M(1400), M(1200), M(800), M(450) };
static const Rectangle OB_FLOOR_STORAGE = { M(1400), M(1650), M(800), M(550) };
static const Rectangle OB_FLOOR_RIGHT_BACK_OUTLINE = { M(1400), M(1200), M(800), M(1000) };

static void BuildGeometry(Map *map);
static void BuildInteractables(Map *map);
static void BuildNpcs(Map *map);

static bool IsRedMaskColor(Color c) {
    return (c.r > 200 && c.g < 90 && c.b < 90);
}

bool Map_RectBlocked(const Map *map, Rectangle r) {
    if (!map || !map->collisionGrid || map->collGridW <= 0 || map->collGridH <= 0)
        return false;

    if (r.x + r.width <= 0.0f || r.y + r.height <= 0.0f
        || r.x >= WORLD_WIDTH || r.y >= WORLD_HEIGHT)
        return true;

    float cw = map->cellWorldW;
    float ch = map->cellWorldH;
    if (cw <= 0.0f || ch <= 0.0f)
        return false;

    int gx0 = (int)floorf(r.x / cw);
    int gy0 = (int)floorf(r.y / ch);
    int gx1 = (int)floorf((r.x + r.width - 1e-4f) / cw);
    int gy1 = (int)floorf((r.y + r.height - 1e-4f) / ch);

    if (gx0 < 0) gx0 = 0;
    if (gy0 < 0) gy0 = 0;
    if (gx1 >= map->collGridW) gx1 = map->collGridW - 1;
    if (gy1 >= map->collGridH) gy1 = map->collGridH - 1;

    for (int gy = gy0; gy <= gy1; gy++) {
        for (int gx = gx0; gx <= gx1; gx++) {
            if (map->collisionGrid[gy * map->collGridW + gx])
                return true;
        }
    }
    return false;
}

static void Map_LoadCollisionMask(Map *map) {
    map->collisionGrid = NULL;
    map->collGridW = 0;
    map->collGridH = 0;
    map->cellWorldW = 0.0f;
    map->cellWorldH = 0.0f;

    const char *path = "assets/collision_mask.png";
    if (!FileExists(path))
        return;

    Image img = LoadImage(path);
    if (img.width <= 0 || img.height <= 0 || img.data == NULL) {
        if (img.data != NULL) UnloadImage(img);
        return;
    }

    const int MAX_DIM = 1024;
    int nw = img.width;
    int nh = img.height;
    if (nw > MAX_DIM || nh > MAX_DIM) {
        float s = fminf((float)MAX_DIM / (float)nw, (float)MAX_DIM / (float)nh);
        nw = (int)((float)nw * s);
        nh = (int)((float)nh * s);
        if (nw < 1) nw = 1;
        if (nh < 1) nh = 1;
        ImageResize(&img, nw, nh);
    }

    size_t cells = (size_t)nw * (size_t)nh;
    unsigned char *grid = (unsigned char *)calloc(cells, 1);
    if (!grid) {
        UnloadImage(img);
        return;
    }

    for (int y = 0; y < nh; y++) {
        for (int x = 0; x < nw; x++) {
            Color c = GetImageColor(img, x, y);
            if (IsRedMaskColor(c))
                grid[y * nw + x] = 1;
        }
    }

    UnloadImage(img);

    map->collisionGrid = grid;
    map->collGridW = nw;
    map->collGridH = nh;
    map->cellWorldW = WORLD_WIDTH / (float)nw;
    map->cellWorldH = WORLD_HEIGHT / (float)nh;
}

/* World size constants are in map.h — used for camera clamping in game.c. */
float Map_GetWorldWidth(void)  { return WORLD_WIDTH; }
float Map_GetWorldHeight(void) { return WORLD_HEIGHT; }

Map *Map_Create(int screenWidth, int screenHeight) {
    (void)screenWidth;
    (void)screenHeight;
    Map *m = (Map *)malloc(sizeof(Map));
    if (!m) return NULL;

    m->bounds = (Rectangle){ 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT };
    m->collisionGrid = NULL;
    m->collGridW = 0;
    m->collGridH = 0;
    m->cellWorldW = 0.0f;
    m->cellWorldH = 0.0f;
    m->interactableCount = 0;
    m->npcCount = 0;
    m->bloodMoon = false;
    m->backgroundTexture = (Texture2D){ 0 };

    BuildGeometry(m);
    BuildInteractables(m);
    BuildNpcs(m);
    Map_LoadCollisionMask(m);

    if (FileExists("assets/map_blueprint.png")) {
        m->backgroundTexture = LoadTexture("assets/map_blueprint.png");
    }

    return m;
}

void Map_Destroy(Map *map) {
    if (!map) return;
    if (map->backgroundTexture.id != 0) {
        UnloadTexture(map->backgroundTexture);
    }
    free(map->collisionGrid);
    free(map);
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
// Blueprint layout (design units; multiplied by WORLD_SCALE at build time):
// Main store: 0..2800 x 0..1200
// Middle: 0..2800 x 1200..2200
// Basement: 0..2800 x 2200..3800
static void BuildGeometry(Map *map) {
    float W = WORLD_WIDTH;

    map->bounds = (Rectangle){ 0, 0, W, WORLD_HEIGHT };

    // Region bounds
    map->kitchenBounds   = (Rectangle){ 0, M(1200), M(1100), M(1000) };
    map->hallwayBounds   = (Rectangle){ M(1100), M(1200), M(300), M(1000) };
    map->cashierBounds   = (Rectangle){ M(2200), 0, M(600), M(450) };
    map->freezerBounds   = (Rectangle){ M(50), M(2050), M(550), M(130) };
    map->basementBounds  = (Rectangle){ 0, M(2200), W, M(1600) };
}

// Code created by wu deguang - each interactable has bounds (object) and triggerZone (where to stand to press E)
static void BuildInteractables(Map *map) {
    // Badge reader (kitchen, near hallway entrance) - clock in / clock out
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(950), M(1280), M(100), M(60) },
        (Rectangle){ M(850), M(1300), M(110), M(80) },
        INTERACT_BADGE,
        "Badge"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(950), M(1340), M(100), M(45) },
        (Rectangle){ M(850), M(1360), M(110), M(70) },
        INTERACT_CLOCK_OUT,
        "Clock Out"
    };
    // Sink (kitchen top-left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(80), M(1210), M(220), M(140) },
        (Rectangle){ M(80), M(1360), M(240), M(70) },
        INTERACT_SINK,
        "Sink"
    };
    // Garbage (kitchen left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(120), M(1950), M(100), M(130) },
        (Rectangle){ M(100), M(2090), M(140), M(55) },
        INTERACT_GARBAGE,
        "Bin"
    };
    // Mop (janitor far right)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(2050), M(1580), M(130), M(60) },
        (Rectangle){ M(1920), M(1590), M(130), M(80) },
        INTERACT_MOP,
        "Mop"
    };
    // Radio on cashier counter (main store top-right) - Day 2 turn on radio; customers use cashierBounds in game logic
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(2580), M(120), M(80), M(60) },
        (Rectangle){ M(2560), M(200), M(120), M(100) },
        INTERACT_RADIO,
        "Radio"
    };
    // Freezer (kitchen bottom-left, expired meat)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(50), M(2050), M(230), M(130) },
        (Rectangle){ M(50), M(2190), M(230), M(65) },
        INTERACT_FREEZER_DOOR,
        "Freezer"
    };
    // Basement stairs down (hallway bottom)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(1100), M(2120), M(300), M(80) },
        (Rectangle){ M(1150), M(2080), M(200), M(55) },
        INTERACT_BASEMENT_STAIRS,
        "Stairs"
    };
    // Basement stairs up
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(1100), M(2200), M(300), M(180) },
        (Rectangle){ M(1150), M(2390), M(200), M(65) },
        INTERACT_BASEMENT_STAIRS_UP,
        "Stairs Up"
    };
    // Generator (basement left)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(200), M(2350), M(450), M(500) },
        (Rectangle){ M(200), M(2860), M(450), M(85) },
        INTERACT_GENERATOR,
        "Generator"
    };
    // Lockers (basement right)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(2080), M(2500), M(140), M(620) },
        (Rectangle){ M(2080), M(3130), M(140), M(65) },
        INTERACT_LOCKER_1,
        "Locker1"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(2220), M(2500), M(140), M(620) },
        (Rectangle){ M(2220), M(3130), M(140), M(65) },
        INTERACT_LOCKER_2,
        "Locker2"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ M(2360), M(2500), M(140), M(620) },
        (Rectangle){ M(2360), M(3130), M(140), M(65) },
        INTERACT_LOCKER_3,
        "Locker3"
    };
}

/* Placeholder silhouettes at cashier; game logic picks which customer by day when you press E in cashier area. */
static void BuildNpcs(Map *map) {
    float cx = M(2550);
    float cy = M(350);
    float cw = M(80);
    float ch = M(100);
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
        DrawRectangleRec(OB_FLOOR_MAIN, (Color){ 35, 32, 42, 255 });
        DrawRectangleRec(map->kitchenBounds, (Color){ 28, 30, 40, 255 });
        DrawRectangleLinesEx(map->kitchenBounds, 2, (Color){ 70, 75, 95, 255 });
        DrawRectangleRec(map->hallwayBounds, (Color){ 20, 18, 32, 255 });
        DrawRectangleRec(OB_FLOOR_JANITOR, (Color){ 32, 28, 38, 255 });
        DrawRectangleRec(OB_FLOOR_STORAGE, (Color){ 30, 28, 40, 255 });
        DrawRectangleLinesEx(OB_FLOOR_RIGHT_BACK_OUTLINE, 1, (Color){ 65, 60, 85, 255 });
        DrawRectangleRec(map->basementBounds, (Color){ 14, 12, 22, 255 });
        DrawRectangleLinesEx(map->basementBounds, 2, (Color){ 45, 42, 60, 255 });

        Color shelfColor = (Color){ 55, 50, 65, 255 };
        Color shelfLine = (Color){ 90, 85, 110, 255 };
        if (rect_center_y(OB_SHELF_NW) < playerY) DrawOneShelf(OB_SHELF_NW, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_NE) < playerY) DrawOneShelf(OB_SHELF_NE, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_SW) < playerY) DrawOneShelf(OB_SHELF_SW, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_SE) < playerY) DrawOneShelf(OB_SHELF_SE, shelfColor, shelfLine);
        if (rect_center_y(OB_COUNTER) < playerY) {
            DrawRectangleRec(OB_COUNTER, (Color){ 60, 50, 45, 255 });
            DrawRectangleLinesEx(OB_COUNTER, 2, (Color){ 100, 90, 80, 255 });
        }
        if (rect_center_y(OB_SINK_ROW) < playerY) DrawRectangleRec(OB_SINK_ROW, (Color){ 70, 72, 78, 255 });
        if (rect_center_y(OB_TABLE) < playerY) DrawRectangleRec(OB_TABLE, (Color){ 55, 45, 38, 255 });
        if (rect_center_y(OB_BIN) < playerY) DrawRectangleRec(OB_BIN, (Color){ 40, 38, 42, 255 });
        if (rect_center_y(OB_FREEZER_L) < playerY) {
            DrawRectangleRec(OB_FREEZER_L, (Color){ 25, 35, 55, 255 });
            DrawRectangleLinesEx(OB_FREEZER_L, 1, (Color){ 60, 90, 130, 255 });
        }
        if (rect_center_y(OB_FREEZER_M) < playerY) DrawRectangleRec(OB_FREEZER_M, (Color){ 30, 38, 52, 255 });
        if (rect_center_y(OB_JANITOR_CUP) < playerY) DrawRectangleRec(OB_JANITOR_CUP, (Color){ 50, 42, 38, 255 });
        if (rect_center_y(OB_MOP) < playerY) DrawRectangleRec(OB_MOP, (Color){ 45, 55, 70, 255 });
        if (rect_center_y(OB_STORAGE_CRATES) < playerY) DrawRectangleRec(OB_STORAGE_CRATES, (Color){ 48, 40, 35, 255 });
        if (rect_center_y(OB_STORAGE_MACHINE) < playerY) DrawRectangleRec(OB_STORAGE_MACHINE, (Color){ 35, 38, 42, 255 });
        if (rect_center_y(OB_GENERATOR) < playerY) {
            DrawRectangleRec(OB_GENERATOR, (Color){ 42, 48, 45, 255 });
            DrawRectangleLinesEx(OB_GENERATOR, 1, (Color){ 70, 85, 75, 255 });
        }
        if (rect_center_y(OB_LOCKER_1) < playerY) DrawRectangleRec(OB_LOCKER_1, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKER_2) < playerY) DrawRectangleRec(OB_LOCKER_2, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKER_3) < playerY) DrawRectangleRec(OB_LOCKER_3, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKERS_BBOX) < playerY) DrawRectangleLinesEx(OB_LOCKERS_BBOX, 1, (Color){ 75, 78, 95, 255 });
        if (rect_center_y(OB_STAIRS_DOWN) < playerY) DrawRectangleRec(OB_STAIRS_DOWN, (Color){ 50, 45, 55, 255 });
        if (rect_center_y(OB_STAIRS_UP) < playerY) DrawRectangleRec(OB_STAIRS_UP, (Color){ 35, 32, 42, 255 });
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
        DrawCircle((int)M(2720), (int)M(80), M(40), (Color){ 80, 20, 20, 220 });
        DrawCircle((int)M(2720), (int)M(80), M(28), (Color){ 140, 40, 40, 255 });
    }
}

// Code created by wu deguang - draw objects in front of player (Y-sort); skip when using image
void Map_DrawForeground(const Map *map, float playerY) {
    if (map->backgroundTexture.id == 0) {
        Color shelfColor = (Color){ 55, 50, 65, 255 };
        Color shelfLine = (Color){ 90, 85, 110, 255 };
        if (rect_center_y(OB_SHELF_NW) >= playerY) DrawOneShelf(OB_SHELF_NW, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_NE) >= playerY) DrawOneShelf(OB_SHELF_NE, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_SW) >= playerY) DrawOneShelf(OB_SHELF_SW, shelfColor, shelfLine);
        if (rect_center_y(OB_SHELF_SE) >= playerY) DrawOneShelf(OB_SHELF_SE, shelfColor, shelfLine);
        if (rect_center_y(OB_COUNTER) >= playerY) {
            DrawRectangleRec(OB_COUNTER, (Color){ 60, 50, 45, 255 });
            DrawRectangleLinesEx(OB_COUNTER, 2, (Color){ 100, 90, 80, 255 });
        }
        if (rect_center_y(OB_SINK_ROW) >= playerY) DrawRectangleRec(OB_SINK_ROW, (Color){ 70, 72, 78, 255 });
        if (rect_center_y(OB_TABLE) >= playerY) DrawRectangleRec(OB_TABLE, (Color){ 55, 45, 38, 255 });
        if (rect_center_y(OB_BIN) >= playerY) DrawRectangleRec(OB_BIN, (Color){ 40, 38, 42, 255 });
        if (rect_center_y(OB_FREEZER_L) >= playerY) {
            DrawRectangleRec(OB_FREEZER_L, (Color){ 25, 35, 55, 255 });
            DrawRectangleLinesEx(OB_FREEZER_L, 1, (Color){ 60, 90, 130, 255 });
        }
        if (rect_center_y(OB_FREEZER_M) >= playerY) DrawRectangleRec(OB_FREEZER_M, (Color){ 30, 38, 52, 255 });
        if (rect_center_y(OB_JANITOR_CUP) >= playerY) DrawRectangleRec(OB_JANITOR_CUP, (Color){ 50, 42, 38, 255 });
        if (rect_center_y(OB_MOP) >= playerY) DrawRectangleRec(OB_MOP, (Color){ 45, 55, 70, 255 });
        if (rect_center_y(OB_STORAGE_CRATES) >= playerY) DrawRectangleRec(OB_STORAGE_CRATES, (Color){ 48, 40, 35, 255 });
        if (rect_center_y(OB_STORAGE_MACHINE) >= playerY) DrawRectangleRec(OB_STORAGE_MACHINE, (Color){ 35, 38, 42, 255 });
        if (rect_center_y(OB_GENERATOR) >= playerY) {
            DrawRectangleRec(OB_GENERATOR, (Color){ 42, 48, 45, 255 });
            DrawRectangleLinesEx(OB_GENERATOR, 1, (Color){ 70, 85, 75, 255 });
        }
        if (rect_center_y(OB_LOCKER_1) >= playerY) DrawRectangleRec(OB_LOCKER_1, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKER_2) >= playerY) DrawRectangleRec(OB_LOCKER_2, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKER_3) >= playerY) DrawRectangleRec(OB_LOCKER_3, (Color){ 38, 40, 48, 255 });
        if (rect_center_y(OB_LOCKERS_BBOX) >= playerY) DrawRectangleLinesEx(OB_LOCKERS_BBOX, 1, (Color){ 75, 78, 95, 255 });
        if (rect_center_y(OB_STAIRS_DOWN) >= playerY) DrawRectangleRec(OB_STAIRS_DOWN, (Color){ 50, 45, 55, 255 });
        if (rect_center_y(OB_STAIRS_UP) >= playerY) DrawRectangleRec(OB_STAIRS_UP, (Color){ 35, 32, 42, 255 });
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
