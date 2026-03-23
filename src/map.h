/*
 * ============================================================================
 * map.h — WORLD SIZE, INTERACTABLE TYPES, MAP STRUCT
 * ============================================================================
 * WORLD_WIDTH / HEIGHT — total playable area in the same units as player position.
 * InteractableType — each number names one kind of object (sink, radio, stairs...).
 * ============================================================================
 */

#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include <stdbool.h>

/* Code created by wu deguang — large world so one room fills most of the window */
#define WORLD_WIDTH   2800.0f
#define WORLD_HEIGHT  3800.0f

#define MAX_WALLS        128
#define MAX_INTERACTABLES 20
#define MAX_NPCS          8

typedef enum InteractableType {
    INTERACT_BADGE = 0,
    INTERACT_SINK,
    INTERACT_MOP,
    INTERACT_RADIO,
    INTERACT_GARBAGE,
    INTERACT_FREEZER_DOOR,
    INTERACT_BASEMENT_STAIRS,
    INTERACT_BASEMENT_STAIRS_UP,
    INTERACT_GENERATOR,
    INTERACT_LOCKER_1,
    INTERACT_LOCKER_2,
    INTERACT_LOCKER_3,
    INTERACT_CLOCK_OUT
} InteractableType;

typedef struct Interactable {
    Rectangle bounds;       // Object / collision footprint (for drawing and blocking)
    Rectangle triggerZone;  // Where player stands to press E (in front of object)
    InteractableType type;
    const char *label;
} Interactable;

typedef enum NpcType {
    NPC_YOUNG_LADY = 0,
    NPC_OLD_MAN_EYEPATCH,
    NPC_TEEN_BOY,
    NPC_OLD_LADY,
    NPC_CREEPY_MAN,
    NPC_SHAMAN
} NpcType;

typedef struct NpcSpot {
    Rectangle bounds;
    NpcType type;
    const char *label;
} NpcSpot;

typedef enum MapRegion {
    REGION_KITCHEN = 0,
    REGION_HALLWAY,
    REGION_CASHIER,
    REGION_FREEZER,
    REGION_BASEMENT
} MapRegion;

typedef struct Map {
    Rectangle bounds;
    Rectangle walls[MAX_WALLS];
    int wallCount;

    Interactable interactables[MAX_INTERACTABLES];
    int interactableCount;

    NpcSpot npcs[MAX_NPCS];
    int npcCount;

    // Region bounds for trigger checks (e.g. is player in basement?)
    Rectangle kitchenBounds;
    Rectangle hallwayBounds;
    Rectangle cashierBounds;
    Rectangle freezerBounds;
    Rectangle basementBounds;

    // Visual: blood moon on Day 4
    bool bloodMoon;

    // Code created by wu deguang
    // Optional: blueprint image used as background (if loaded)
    Texture2D backgroundTexture;
} Map;

Map  *Map_Create(int screenWidth, int screenHeight);
void  Map_Destroy(Map *map);

float Map_GetWorldWidth(void);
float Map_GetWorldHeight(void);

const Rectangle     *Map_GetWalls(const Map *map, int *count);
const Interactable   *Map_GetInteractables(const Map *map, int *count);
const NpcSpot        *Map_GetNpcs(const Map *map, int *count);

MapRegion Map_GetRegionAt(const Map *map, float x, float y);

void  Map_DrawBackground(const Map *map, float playerY);
void  Map_DrawForeground(const Map *map, float playerY);

#endif /* MAP_H */
