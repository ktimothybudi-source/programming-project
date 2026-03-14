#include "map.h"
#include "game.h"
#include "raylib.h"
#include <stdlib.h>

static void BuildGeometry(Map *map, int screenWidth, int screenHeight);
static void BuildInteractables(Map *map);
static void BuildNpcs(Map *map);

Map *Map_Create(int screenWidth, int screenHeight) {
    Map *m = (Map *)malloc(sizeof(Map));
    if (!m) return NULL;

    m->bounds = (Rectangle){ 0.0f, 0.0f, (float)screenWidth, (float)screenHeight };
    m->wallCount = 0;
    m->interactableCount = 0;
    m->npcCount = 0;
    m->bloodMoon = false;

    BuildGeometry(m, screenWidth, screenHeight);
    BuildInteractables(m);
    BuildNpcs(m);

    return m;
}

void Map_Destroy(Map *map) {
    if (map) free(map);
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

MapRegion Map_GetRegionAt(const Map *map, float x, float y) {
    if (CheckCollisionPointRec((Vector2){ x, y }, map->basementBounds)) return REGION_BASEMENT;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->freezerBounds)) return REGION_FREEZER;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->kitchenBounds)) return REGION_KITCHEN;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->cashierBounds)) return REGION_CASHIER;
    if (CheckCollisionPointRec((Vector2){ x, y }, map->hallwayBounds)) return REGION_HALLWAY;
    return REGION_HALLWAY;
}

static void BuildGeometry(Map *map, int screenWidth, int screenHeight) {
    float m = 20.0f;
    float w = (float)screenWidth - 2.0f * m;
    float h = (float)screenHeight - 2.0f * m;

    // Main play area
    map->bounds = (Rectangle){ m, m, w, h };

    // ---- Regions (for trigger logic) ----
    // Kitchen: left third, backdoor + sink
    map->kitchenBounds = (Rectangle){ m, m, w * 0.28f, h * 0.5f };
    // Hallway: middle strip
    map->hallwayBounds = (Rectangle){ m + w * 0.28f, m, w * 0.22f, h };
    // Cashier / main floor: right side
    map->cashierBounds = (Rectangle){ m + w * 0.5f, m, w * 0.5f - 20.0f, h * 0.65f };
    // Freezer: small room off kitchen
    map->freezerBounds = (Rectangle){ m + 30.0f, m + h * 0.52f, 100.0f, 90.0f };
    // Basement: bottom area, reachable by stairs
    map->basementBounds = (Rectangle){ m + w * 0.5f - 60.0f, m + h * 0.7f, 220.0f, h * 0.28f - 20.0f };

    // ---- Walls: outer ----
    map->walls[map->wallCount++] = (Rectangle){ m, m, w, 10.0f };
    map->walls[map->wallCount++] = (Rectangle){ m, m + h - 10.0f, w, 10.0f };
    map->walls[map->wallCount++] = (Rectangle){ m, m, 10.0f, h };
    map->walls[map->wallCount++] = (Rectangle){ m + w - 10.0f, m, 10.0f, h };

    // Kitchen / hallway divider
    map->walls[map->wallCount++] = (Rectangle){ m + w * 0.28f, m, 8.0f, h * 0.52f };
    // Hallway / cashier divider (with gap for door)
    map->walls[map->wallCount++] = (Rectangle){ m + w * 0.5f - 10.0f, m, 10.0f, h * 0.35f };
    map->walls[map->wallCount++] = (Rectangle){ m + w * 0.5f - 10.0f, m + h * 0.45f, 10.0f, h * 0.25f };
    // Freezer walls
    map->walls[map->wallCount++] = (Rectangle){ map->freezerBounds.x, map->freezerBounds.y, map->freezerBounds.width, 8.0f };
    map->walls[map->wallCount++] = (Rectangle){ map->freezerBounds.x, map->freezerBounds.y + map->freezerBounds.height - 8.0f, map->freezerBounds.width, 8.0f };
    map->walls[map->wallCount++] = (Rectangle){ map->freezerBounds.x, map->freezerBounds.y, 8.0f, map->freezerBounds.height };
    map->walls[map->wallCount++] = (Rectangle){ map->freezerBounds.x + map->freezerBounds.width - 8.0f, map->freezerBounds.y, 8.0f, map->freezerBounds.height };
    // Basement walls (pit)
    map->walls[map->wallCount++] = (Rectangle){ map->basementBounds.x, map->basementBounds.y, map->basementBounds.width, 10.0f };
    map->walls[map->wallCount++] = (Rectangle){ map->basementBounds.x, map->basementBounds.y + map->basementBounds.height - 10.0f, map->basementBounds.width, 10.0f };
    map->walls[map->wallCount++] = (Rectangle){ map->basementBounds.x, map->basementBounds.y, 10.0f, map->basementBounds.height };
    map->walls[map->wallCount++] = (Rectangle){ map->basementBounds.x + map->basementBounds.width - 10.0f, map->basementBounds.y, 10.0f, map->basementBounds.height };

    // Counters / obstacles in kitchen
    map->walls[map->wallCount++] = (Rectangle){ m + 50.0f, m + 120.0f, 80.0f, 25.0f };
    map->walls[map->wallCount++] = (Rectangle){ m + 50.0f, m + 200.0f, 100.0f, 20.0f };
    // Cashier counter
    map->walls[map->wallCount++] = (Rectangle){ m + w * 0.52f, m + h * 0.4f, 180.0f, 22.0f };
}

static void BuildInteractables(Map *map) {
    float m = map->bounds.x;
    float w = map->bounds.width;
    float h = map->bounds.height;

    // Badge reader (kitchen, near backdoor)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + 70.0f, m + 50.0f, 36.0f, 28.0f },
        INTERACT_BADGE,
        "Badge"
    };
    // Sink (kitchen)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + 120.0f, m + 200.0f, 50.0f, 35.0f },
        INTERACT_SINK,
        "Sink"
    };
    // Mop bucket (hallway or cashier area)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + w * 0.32f, m + h * 0.3f, 32.0f, 40.0f },
        INTERACT_MOP,
        "Mop"
    };
    // Radio (cashier counter)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + w * 0.55f, m + h * 0.35f, 40.0f, 30.0f },
        INTERACT_RADIO,
        "Radio"
    };
    // Garbage bin (outside or kitchen exit)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + w * 0.48f, m + h * 0.62f, 36.0f, 40.0f },
        INTERACT_GARBAGE,
        "Bin"
    };
    // Freezer door
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->freezerBounds.x + 20.0f, map->freezerBounds.y - 5.0f, 50.0f, 14.0f },
        INTERACT_FREEZER_DOOR,
        "Freezer"
    };
    // Basement stairs (top of stairs - go down)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + w * 0.52f, m + h * 0.66f, 60.0f, 25.0f },
        INTERACT_BASEMENT_STAIRS,
        "Stairs"
    };
    // Basement stairs (from inside basement - go up)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->basementBounds.x + map->basementBounds.width * 0.5f - 35.0f, map->basementBounds.y - 5.0f, 50.0f, 20.0f },
        INTERACT_BASEMENT_STAIRS_UP,
        "Stairs Up"
    };
    // Generator (basement)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->basementBounds.x + 30.0f, map->basementBounds.y + 25.0f, 50.0f, 45.0f },
        INTERACT_GENERATOR,
        "Generator"
    };
    // Lockers (basement, three)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->basementBounds.x + 100.0f, map->basementBounds.y + 20.0f, 28.0f, 55.0f },
        INTERACT_LOCKER_1,
        "Locker1"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->basementBounds.x + 135.0f, map->basementBounds.y + 20.0f, 28.0f, 55.0f },
        INTERACT_LOCKER_2,
        "Locker2"
    };
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ map->basementBounds.x + 170.0f, map->basementBounds.y + 20.0f, 28.0f, 55.0f },
        INTERACT_LOCKER_3,
        "Locker3"
    };
    // Clock out (kitchen, near badge)
    map->interactables[map->interactableCount++] = (Interactable){
        (Rectangle){ m + 70.0f, m + 85.0f, 36.0f, 22.0f },
        INTERACT_CLOCK_OUT,
        "Clock Out"
    };
}

static void BuildNpcs(Map *map) {
    float m = map->bounds.x;
    float w = map->bounds.width;
    float h = map->bounds.height;

    // Customer positions at cashier (game will show/hide by day)
    // Young lady - Day 2
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_YOUNG_LADY,
        "Lady"
    };
    // Old man eyepatch - Day 2
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_OLD_MAN_EYEPATCH,
        "Old Man"
    };
    // Teen boy - Day 3
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_TEEN_BOY,
        "Teen"
    };
    // Old lady (panic) - Day 3
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_OLD_LADY,
        "Old Lady"
    };
    // Creepy man (maggots) - Day 3
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_CREEPY_MAN,
        "Creepy"
    };
    // Shaman - Day 4
    map->npcs[map->npcCount++] = (NpcSpot){
        (Rectangle){ m + w * 0.72f, m + h * 0.42f, 22.0f, 36.0f },
        NPC_SHAMAN,
        "Shaman"
    };
}

void Map_DrawBackground(const Map *map, const void *game) {
    const Game *g = (const Game *)game;
    (void)g;

    ClearBackground((Color){ 8, 6, 18, 255 });

    // Ground - main area
    DrawRectangleRec(map->bounds, (Color){ 22, 22, 38, 255 });

    // Kitchen - white/black tile feel (darker)
    DrawRectangleRec(map->kitchenBounds, (Color){ 30, 28, 45, 255 });
    DrawRectangleLinesEx(map->kitchenBounds, 2.0f, (Color){ 80, 75, 100, 255 });

    // Hallway
    DrawRectangleRec(map->hallwayBounds, (Color){ 18, 16, 32, 255 });

    // Cashier / main floor
    DrawRectangleRec(map->cashierBounds, (Color){ 28, 26, 42, 255 });
    DrawRectangleLinesEx(map->cashierBounds, 1.0f, (Color){ 70, 65, 90, 255 });

    // Freezer (cold blue tint)
    DrawRectangleRec(map->freezerBounds, (Color){ 20, 30, 55, 255 });
    DrawRectangleLinesEx(map->freezerBounds, 2.0f, (Color){ 60, 90, 140, 255 });

    // Basement (darkest)
    DrawRectangleRec(map->basementBounds, (Color){ 12, 10, 22, 255 });
    DrawRectangleLinesEx(map->basementBounds, 2.0f, (Color){ 50, 45, 70, 255 });

    // Interactables as labeled boxes
    for (int i = 0; i < map->interactableCount; i++) {
        const Interactable *in = &map->interactables[i];
        Color c = (Color){ 50, 45, 70, 255 };
        if (in->type == INTERACT_BADGE || in->type == INTERACT_CLOCK_OUT) c = (Color){ 60, 50, 80, 255 };
        else if (in->type == INTERACT_RADIO) c = (Color){ 70, 40, 40, 255 };
        else if (in->type == INTERACT_GENERATOR) c = (Color){ 45, 50, 45, 255 };
        else if (in->type == INTERACT_LOCKER_1 || in->type == INTERACT_LOCKER_2 || in->type == INTERACT_LOCKER_3)
            c = (Color){ 35, 35, 45, 255 };
        DrawRectangleRounded(in->bounds, 0.15f, 4, c);
        DrawRectangleLinesEx(in->bounds, 1.5f, (Color){ 120, 115, 140, 255 });
        if (in->label)
            DrawText(in->label, (int)(in->bounds.x + 4), (int)(in->bounds.y - 14), 11, (Color){ 200, 195, 220, 255 });
    }

    // NPCs (when visible - game controls which are active per day; we draw all positions, game can pass day and we only draw the right one, or game draws NPCs)
    for (int i = 0; i < map->npcCount; i++) {
        const NpcSpot *n = &map->npcs[i];
        DrawRectangleRounded(n->bounds, 0.25f, 4, (Color){ 18, 14, 28, 255 });
        DrawRectangleLinesEx(n->bounds, 1.0f, (Color){ 90, 85, 110, 255 });
    }

    // Blood moon through window (Day 4) - small window effect top right
    if (map->bloodMoon) {
        DrawCircle(map->bounds.x + map->bounds.width - 50.0f, map->bounds.y + 45.0f, 18.0f, (Color){ 80, 20, 20, 220 });
        DrawCircle(map->bounds.x + map->bounds.width - 50.0f, map->bounds.y + 45.0f, 14.0f, (Color){ 140, 40, 40, 255 });
    }
}

void Map_DrawForeground(const Map *map) {
    (void)map;
    // Vignette optional
}
