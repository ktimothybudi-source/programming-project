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
./ *   1) collisionGrid — from assets/new_collision_map.png (#FFAEC9 pink only = blocked).
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
/* Reference image pixels (Badge_Clock_out...png / map_blueprint dimensions). */
#define REF_IMG_W 682.0f
#define REF_IMG_H 1024.0f
#define RX(x) (((float)(x) / REF_IMG_W) * WORLD_WIDTH)
#define RY(y) (((float)(y) / REF_IMG_H) * WORLD_HEIGHT)
#define RR(x, y, w, h) (Rectangle){ RX(x), RY(y), RX(x + (w)) - RX(x), RY(y + (h)) - RY(y) }

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
static void Map_LoadTaskZonesFromMask(Map *map);
static void Map_LoadNpcTextures(Map *map);
static void DrawNpcSprite(const Map *map, NpcType type, Rectangle r);
static void DrawNpcGroundShadow(float cx, float footY, float refWidth);
static void DrawNpcsForSort(const Map *map, float playerY, bool drawBehind);

/*
 * Collision mask: ONLY pastel pink #FFAEC9 (255,174,201) used in new_collision_map.png.
 * Squared RGB distance keeps anti-aliased edge pixels; no broad "magenta" rule (that
 * matched unrelated purples / shadows and caused stray hitboxes).
 */
static bool IsPinkCollisionMaskColor(Color c) {
    int dr = (int)c.r - 255;
    int dg = (int)c.g - 174;
    int db = (int)c.b - 201;
    int dist2 = dr * dr + dg * dg + db * db;
    return dist2 <= 520;
}

static bool IsNearColor(Color c, Color t, int tol) {
    return abs((int)c.r - (int)t.r) <= tol
        && abs((int)c.g - (int)t.g) <= tol
        && abs((int)c.b - (int)t.b) <= tol;
}

static Interactable *FindInteractableByType(Map *map, InteractableType type) {
    for (int i = 0; i < map->interactableCount; i++) {
        if (map->interactables[i].type == type) return &map->interactables[i];
    }
    return NULL;
}

static bool FindMaskRegion(Image *img, Color target, Rectangle *outPx) {
    const int tol = 12;
    int minX = img->width, minY = img->height;
    int maxX = -1, maxY = -1;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Color c = GetImageColor(*img, x, y);
            if (!IsNearColor(c, target, tol)) continue;
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
    }

    if (maxX < minX || maxY < minY) return false;
    *outPx = (Rectangle){ (float)minX, (float)minY, (float)(maxX - minX + 1), (float)(maxY - minY + 1) };
    return true;
}

static Rectangle PxRectToWorld(Rectangle rPx, int imgW, int imgH) {
    float x = (rPx.x / (float)imgW) * WORLD_WIDTH;
    float y = (rPx.y / (float)imgH) * WORLD_HEIGHT;
    float w = (rPx.width / (float)imgW) * WORLD_WIDTH;
    float h = (rPx.height / (float)imgH) * WORLD_HEIGHT;
    return (Rectangle){ x, y, w, h };
}

static Rectangle BuildDefaultTriggerFromBounds(Rectangle b) {
    float padX = b.width * 0.12f;
    float h = fmaxf(14.0f, b.height * 0.5f);
    return (Rectangle){ b.x - padX, b.y + b.height, b.width + padX * 2.0f, h };
}

static void Map_LoadTaskZonesFromMask(Map *map) {
    const char *path = "assets/task_mask.png";
    if (!FileExists(path)) return;

    Image img = LoadImage(path);
    if (!img.data || img.width <= 0 || img.height <= 0) {
        if (img.data) UnloadImage(img);
        return;
    }

    Rectangle px = {0};

    // Blue maps to Badge + Clock Out + Radio (split into three stacked bands).
    if (FindMaskRegion(&img, (Color){ 0, 0, 255, 255 }, &px)) {
        Interactable *badge = FindInteractableByType(map, INTERACT_BADGE);
        Interactable *clockOut = FindInteractableByType(map, INTERACT_CLOCK_OUT);
        Interactable *radio = FindInteractableByType(map, INTERACT_RADIO);
        float bandH = px.height / 3.0f;
        Rectangle rBadge = { px.x, px.y, px.width, bandH };
        Rectangle rClock = { px.x, px.y + bandH, px.width, bandH };
        Rectangle rRadio = { px.x, px.y + bandH * 2.0f, px.width, px.height - bandH * 2.0f };
        if (badge) {
            badge->bounds = PxRectToWorld(rBadge, img.width, img.height);
            badge->triggerZone = BuildDefaultTriggerFromBounds(badge->bounds);
        }
        if (clockOut) {
            clockOut->bounds = PxRectToWorld(rClock, img.width, img.height);
            clockOut->triggerZone = BuildDefaultTriggerFromBounds(clockOut->bounds);
        }
        if (radio) {
            radio->bounds = PxRectToWorld(rRadio, img.width, img.height);
            radio->triggerZone = BuildDefaultTriggerFromBounds(radio->bounds);
        }
    }

    // Green = Sink
    if (FindMaskRegion(&img, (Color){ 0, 255, 0, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_SINK);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    // Yellow = Bin
    if (FindMaskRegion(&img, (Color){ 255, 255, 0, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_GARBAGE);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    // Purple = Freezer
    if (FindMaskRegion(&img, (Color){ 128, 0, 255, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_FREEZER_DOOR);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    // Orange = Mop
    if (FindMaskRegion(&img, (Color){ 255, 128, 0, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_MOP);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    // Pink = Generator
    if (FindMaskRegion(&img, (Color){ 255, 0, 255, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_GENERATOR);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    // Red = Lockers (single combined task zone)
    if (FindMaskRegion(&img, (Color){ 255, 0, 0, 255 }, &px)) {
        Interactable *it = FindInteractableByType(map, INTERACT_LOCKER_1);
        if (it) {
            it->bounds = PxRectToWorld(px, img.width, img.height);
            it->triggerZone = BuildDefaultTriggerFromBounds(it->bounds);
        }
    }

    UnloadImage(img);
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

    const char *path = "assets/new_collision_map.png";
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
            if (IsPinkCollisionMaskColor(c))
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
    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        m->npcTextures[i] = (Texture2D){ 0 };
        m->hasNpcTexture[i] = false;
    }

    BuildGeometry(m);
    BuildInteractables(m);
    BuildNpcs(m);
    Map_LoadTaskZonesFromMask(m);
    Map_LoadCollisionMask(m);

    if (FileExists("assets/map_blueprint.png")) {
        m->backgroundTexture = LoadTexture("assets/map_blueprint.png");
    }

    Map_LoadNpcTextures(m);
    return m;
}

void Map_Destroy(Map *map) {
    if (!map) return;
    if (map->backgroundTexture.id != 0) {
        UnloadTexture(map->backgroundTexture);
    }
    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        if (map->hasNpcTexture[i] && map->npcTextures[i].id != 0)
            UnloadTexture(map->npcTextures[i]);
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
    map->cashierBounds   = RR(536, 0, 146, 121);
    map->freezerBounds   = (Rectangle){ M(50), M(2050), M(550), M(130) };
    map->basementBounds  = (Rectangle){ 0, M(2200), W, M(1600) };
}

// Code created by wu deguang - each interactable has bounds (object) and triggerZone (where to stand to press E)
static void BuildInteractables(Map *map) {
    // Placement aligned to task-reference image, with larger practical interaction zones.
    map->interactables[map->interactableCount++] = (Interactable){
        RR(528, 237, 40, 20),
        RR(501, 223, 98, 54),
        INTERACT_BADGE,
        "Badge"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        RR(528, 259, 40, 20),
        RR(501, 245, 98, 54),
        INTERACT_CLOCK_OUT,
        "Clock Out"
    };
    // Sink (kitchen top-left)
    map->interactables[map->interactableCount++] = (Interactable){
        RR(10, 397, 96, 60),
        RR(6, 395, 108, 66),
        INTERACT_SINK,
        "Sink"
    };
    // Garbage (kitchen left)
    map->interactables[map->interactableCount++] = (Interactable){
        RR(4, 463, 76, 84),
        RR(2, 458, 84, 92),
        INTERACT_GARBAGE,
        "Bin"
    };
    // Mop bucket on the right side of janitor room
    map->interactables[map->interactableCount++] = (Interactable){
        RR(558, 368, 116, 128),
        RR(552, 362, 124, 136),
        INTERACT_MOP,
        "Mop"
    };
    // Radio on cashier counter (main store top-right)
    map->interactables[map->interactableCount++] = (Interactable){
        RR(528, 281, 40, 20),
        RR(501, 267, 98, 54),
        INTERACT_RADIO,
        "Radio"
    };
    // Freezer (kitchen bottom-left, expired meat)
    map->interactables[map->interactableCount++] = (Interactable){
        RR(0, 493, 90, 96),
        RR(0, 490, 96, 102),
        INTERACT_FREEZER_DOOR,
        "Freezer"
    };
    // Generator machine (basement left)
    map->interactables[map->interactableCount++] = (Interactable){
        RR(20, 700, 210, 126),
        RR(16, 694, 218, 134),
        INTERACT_GENERATOR,
        "Generator"
    };
    // Lockers (basement right) as one combined interaction.
    map->interactables[map->interactableCount++] = (Interactable){
        RR(490, 694, 148, 154),
        RR(486, 688, 156, 162),
        INTERACT_LOCKER_1,
        "Lockers"
    };
}

/*
 * Customers stand on the main sales floor (y < kitchen).
 * Shelf quads in *design* coords: NW/NE y≈200–480, SW/SE y≈520–800, x≈200–550 and 2250–2600.
 * Spots keep x in the open aisle (≈600–2100) and y high enough that scaled sprites
 * sit on the floor in front of shelves, not on the top fixtures (see screenshot overlap fixes).
 */
static void BuildNpcs(Map *map) {
    static const float NPC_DRAW_SCALE = 1.82f;
    const float nw0 = M(76);
    const float nh0 = M(102);
    const float nw = nw0 * NPC_DRAW_SCALE;
    const float nh = nh0 * NPC_DRAW_SCALE;
    const float dx = (nw - nw0) * 0.5f;
    const float dy = nh - nh0;
    static const struct {
        float x, y;
        const char *label;
    } spots[] = {
        { 660,  708, "Lady" },
        { 920,  706, "Old Man" },
        { 1205, 710, "Teen" },
        { 1500, 708, "Old Lady" },
        { 1780, 706, "Creepy" },
        { 2050, 708, "Shaman" },
    };
    for (size_t i = 0; i < sizeof(spots) / sizeof(spots[0]); i++) {
        map->npcs[map->npcCount++] = (NpcSpot){
            (Rectangle){ M(spots[i].x) - dx, M(spots[i].y) - dy, nw, nh },
            (NpcType)i,
            spots[i].label
        };
    }
}

static inline float rect_center_y(Rectangle r) {
    return r.y + r.height * 0.5f;
}

static void DrawNpcGroundShadow(float cx, float footY, float refWidth) {
    float rx = fmaxf(8.0f, refWidth * 0.44f);
    float ry = fmaxf(5.0f, refWidth * 0.13f);
    DrawEllipse((int)cx, (int)(footY + ry * 0.35f), rx, ry, (Color){ 10, 8, 18, 78 });
}

static void Map_LoadNpcTextures(Map *map) {
    static const char *paths[NPC_TYPE_COUNT] = {
        "assets/sprites/characters/npc_young_lady.png",
        "assets/sprites/characters/npc_old_man.png",
        "assets/sprites/characters/npc_teen_boy.png",
        "assets/sprites/characters/npc_old_lady.png",
        "assets/sprites/characters/npc_creepy_man.png",
        "assets/sprites/characters/npc_shaman.png",
    };
    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        if (FileExists(paths[i])) {
            map->npcTextures[i] = LoadTexture(paths[i]);
            map->hasNpcTexture[i] = (map->npcTextures[i].id != 0);
            if (map->hasNpcTexture[i])
                SetTextureFilter(map->npcTextures[i], TEXTURE_FILTER_POINT);
        }
    }
}

/* Bitmap when available under assets/sprites/characters; else vector art. */
static void DrawNpcSprite(const Map *map, NpcType type, Rectangle r) {
    if (map && (int)type >= 0 && (int)type < NPC_TYPE_COUNT && map->hasNpcTexture[type]) {
        Texture2D tex = map->npcTextures[type];
        float tw = (float)tex.width;
        float th = (float)tex.height;
        float scale = fminf(r.width / tw, r.height / th);
        float dw = tw * scale;
        float dh = th * scale;
        float ox = r.x + (r.width - dw) * 0.5f;
        float oy = r.y + r.height - dh;
        DrawNpcGroundShadow(ox + dw * 0.5f, oy + dh, dw);
        DrawTexturePro(
            tex,
            (Rectangle){ 0, 0, tw, th },
            (Rectangle){ ox, oy, dw, dh },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE);
        return;
    }

    float cx = r.x + r.width * 0.5f;
    DrawNpcGroundShadow(cx, r.y + r.height - 3.0f, r.width);
    float headR = fmaxf(7.0f, r.width * 0.26f);
    float hy = r.y + headR + 3.0f;
    float bodyTop = hy + headR * 0.55f;
    float bodyW = r.width * 0.72f;
    float bodyH = r.y + r.height - bodyTop - 4.0f;
    float bx = cx - bodyW * 0.5f;

    Color skinA = (Color){ 238, 206, 188, 255 };
    Color skinB = (Color){ 222, 188, 168, 255 };
    Color skinC = (Color){ 248, 232, 220, 255 };

    switch (type) {
    case NPC_YOUNG_LADY: {
        Color hair = (Color){ 38, 26, 22, 255 };
        Color dress = (Color){ 175, 85, 125, 255 };
        DrawCircle((int)cx, (int)hy, headR * 1.1f, hair);
        DrawCircle((int)cx, (int)hy, headR, skinA);
        DrawRectangleRounded((Rectangle){ bx - bodyW * 0.06f, bodyTop, bodyW * 1.12f, bodyH }, 0.2f, 6, dress);
        DrawRectangleRounded((Rectangle){ bx - bodyW * 0.12f, bodyTop + bodyH * 0.55f, bodyW * 1.24f, bodyH * 0.48f },
                             0.35f, 6, dress);
    } break;
    case NPC_OLD_MAN_EYEPATCH: {
        Color hair = (Color){ 95, 88, 82, 255 };
        Color coat = (Color){ 95, 75, 55, 255 };
        DrawRectangle((int)(cx - headR * 1.05f), (int)(hy - headR * 0.35f), (int)(headR * 2.1f), (int)(headR * 0.55f), hair);
        DrawCircle((int)cx, (int)hy, headR, skinB);
        DrawRectangle((int)(cx - headR * 0.55f), (int)(hy - 3), 9, 7, (Color){ 20, 18, 22, 255 });
        DrawRectangleRounded((Rectangle){ bx, bodyTop, bodyW, bodyH }, 0.18f, 6, coat);
        DrawRectangleLinesEx((Rectangle){ bx, bodyTop, bodyW, bodyH * 0.35f }, 1, (Color){ 70, 55, 40, 255 });
    } break;
    case NPC_TEEN_BOY: {
        Color cap = (Color){ 55, 95, 160, 255 };
        Color hoodie = (Color){ 70, 110, 175, 255 };
        DrawRectangle((int)(cx - headR * 1.0f), (int)(hy - headR * 0.85f), (int)(headR * 2.0f), (int)(headR * 0.5f), cap);
        DrawCircle((int)cx, (int)hy, headR, skinA);
        DrawRectangleRounded((Rectangle){ bx, bodyTop, bodyW, bodyH }, 0.28f, 6, hoodie);
        DrawRectangle((int)(cx - 1), (int)(bodyTop + bodyH * 0.15f), 2, (int)(bodyH * 0.55f), (Color){ 50, 70, 110, 255 });
    } break;
    case NPC_OLD_LADY: {
        Color bun = (Color){ 120, 105, 95, 255 };
        Color coat = (Color){ 155, 130, 175, 255 };
        DrawCircle((int)(cx + headR * 0.55f), (int)(hy - headR * 0.2f), headR * 0.42f, bun);
        DrawCircle((int)cx, (int)hy, headR, skinB);
        DrawRectangleRounded((Rectangle){ bx, bodyTop, bodyW, bodyH }, 0.22f, 6, coat);
        DrawLineEx((Vector2){ cx + bodyW * 0.35f, bodyTop + bodyH * 0.2f },
                   (Vector2){ cx + bodyW * 0.45f, bodyTop + bodyH * 0.95f }, 2.5f, (Color){ 85, 70, 55, 255 });
    } break;
    case NPC_CREEPY_MAN: {
        Color coat = (Color){ 28, 26, 34, 255 };
        float narrow = bodyW * 0.52f;
        float nbx = cx - narrow * 0.5f;
        DrawCircle((int)cx, (int)hy, headR * 0.95f, (Color){ 215, 210, 225, 255 });
        DrawCircle((int)(cx - headR * 0.25f), (int)(hy - headR * 0.15f), 3, (Color){ 40, 35, 50, 255 });
        DrawCircle((int)(cx + headR * 0.25f), (int)(hy - headR * 0.15f), 3, (Color){ 40, 35, 50, 255 });
        DrawRectangleRounded((Rectangle){ nbx, bodyTop, narrow, bodyH * 1.02f }, 0.12f, 6, coat);
    } break;
    case NPC_SHAMAN:
    default: {
        Color hat = (Color){ 42, 38, 52, 255 };
        Color robe = (Color){ 75, 55, 95, 255 };
        float brimW = r.width * 1.15f;
        DrawRectangle((int)(cx - brimW * 0.5f), (int)(hy - headR * 1.15f), (int)brimW, (int)(headR * 0.45f), hat);
        DrawTriangle((Vector2){ cx, hy - headR * 1.35f },
                     (Vector2){ cx - headR * 0.9f, hy - headR * 0.25f },
                     (Vector2){ cx + headR * 0.9f, hy - headR * 0.25f },
                     (Color){ 55, 48, 68, 255 });
        DrawCircle((int)cx, (int)hy, headR * 0.92f, skinC);
        DrawRectangleRounded((Rectangle){ bx - bodyW * 0.05f, bodyTop, bodyW * 1.1f, bodyH }, 0.2f, 6, robe);
        DrawRectangleLinesEx((Rectangle){ bx - bodyW * 0.05f, bodyTop + bodyH * 0.55f, bodyW * 1.1f, bodyH * 0.2f }, 1,
                             (Color){ 120, 90, 140, 255 });
    } break;
    }
}

static void DrawNpcsForSort(const Map *map, float playerY, bool drawBehind) {
    if (!map) return;
    for (int i = 0; i < map->npcCount; i++) {
        const NpcSpot *n = &map->npcs[i];
        bool behind = rect_center_y(n->bounds) < playerY;
        if (behind != drawBehind) continue;
        DrawNpcSprite(map, n->type, n->bounds);
    }
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
    const int labelFontSize = 26;
    for (int i = 0; i < map->interactableCount; i++) {
        const Interactable *in = &map->interactables[i];
        if (in->label && rect_center_y(in->bounds) < playerY) {
            int tw = MeasureText(in->label, labelFontSize);
            int tx = (int)(in->bounds.x + in->bounds.width * 0.5f - (float)tw * 0.5f);
            int ty = (int)(in->bounds.y - (float)labelFontSize - 8.0f);
            DrawText(in->label, tx, ty, labelFontSize, (Color){ 200, 195, 220, 255 });
        }
    }
    DrawNpcsForSort(map, playerY, true);
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

    const int labelFontSize = 26;
    for (int i = 0; i < map->interactableCount; i++) {
        const Interactable *in = &map->interactables[i];
        if (in->label && rect_center_y(in->bounds) >= playerY) {
            int tw = MeasureText(in->label, labelFontSize);
            int tx = (int)(in->bounds.x + in->bounds.width * 0.5f - (float)tw * 0.5f);
            int ty = (int)(in->bounds.y - (float)labelFontSize - 8.0f);
            DrawText(in->label, tx, ty, labelFontSize, (Color){ 200, 195, 220, 255 });
        }
    }
    DrawNpcsForSort(map, playerY, false);
}
