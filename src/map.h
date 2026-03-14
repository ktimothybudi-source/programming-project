#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_WALLS        64
#define MAX_INTERACTABLES 16
#define MAX_NPCS          8

// Bohou supermarket interactables
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
    Rectangle bounds;
    InteractableType type;
    const char *label;
} Interactable;

// NPCs (customers, shaman, etc.) - type used by game to pick dialogue
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

// Regions for "where is player" (kitchen, hallway, cashier, freezer, basement)
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
} Map;

// Create / destroy
Map  *Map_Create(int screenWidth, int screenHeight);
void  Map_Destroy(Map *map);

// Queries
const Rectangle     *Map_GetWalls(const Map *map, int *count);
const Interactable   *Map_GetInteractables(const Map *map, int *count);
const NpcSpot        *Map_GetNpcs(const Map *map, int *count);

// Which region is this point in?
MapRegion Map_GetRegionAt(const Map *map, float x, float y);

// Drawing (game can be NULL; pass Game* from game.c for day-dependent effects)
void  Map_DrawBackground(const Map *map, const void *game);
void  Map_DrawForeground(const Map *map);

#endif // MAP_H
