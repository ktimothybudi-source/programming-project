 #ifndef MAP_H
 #define MAP_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 #define MAX_WALLS     32
 #define MAX_STALLS    8
 #define MAX_LANTERNS  16
 #define MAX_NPCS      4
 
 typedef enum StallType {
     STALL_FOOD = 0,
     STALL_MASKS,
     STALL_INCENSE,
     STALL_MISC
 } StallType;
 
 typedef struct Stall {
     Rectangle bounds;
     StallType type;
     const char *label;
 } Stall;
 
 typedef struct NpcSpot {
     Rectangle bounds;
     const char *label;
 } NpcSpot;
 
 typedef struct Lantern {
     Vector2 position;
     float radius;
 } Lantern;
 
 typedef struct Map {
     Rectangle bounds;
     Rectangle walls[MAX_WALLS];
     int wallCount;
 
     Stall stalls[MAX_STALLS];
     int stallCount;
 
     NpcSpot npcs[MAX_NPCS];
     int npcCount;
 
     Lantern lanterns[MAX_LANTERNS];
     int lanternCount;
 
     Rectangle uncannyZone;
 
     // Anomaly visual flags
     bool lanternsStrangeColors;
     bool stallShifted;
 } Map;
 
 // Create / destroy
 Map  *Map_Create(int screenWidth, int screenHeight);
 void  Map_Destroy(Map *map);
 
 // Queries
 const Rectangle *Map_GetWalls(const Map *map, int *count);
 const Stall     *Map_GetStalls(const Map *map, int *count);
 const NpcSpot   *Map_GetNpcs(const Map *map, int *count);
 Rectangle        Map_GetUncannyZone(const Map *map);
 
 // Drawing
 void  Map_DrawBackground(const Map *map);
 void  Map_DrawForeground(const Map *map);
 
 #endif // MAP_H
