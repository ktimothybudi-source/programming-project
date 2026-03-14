 #include "map.h"
 #include "raylib.h"
 #include <stdlib.h>
 
 static void BuildGeometry(Map *map, int screenWidth, int screenHeight);
 static void BuildStalls(Map *map);
 static void BuildLanterns(Map *map);
 static void BuildNpcs(Map *map);
 
 Map *Map_Create(int screenWidth, int screenHeight) {
     Map *m = (Map *)malloc(sizeof(Map));
     if (!m) return NULL;
 
     m->bounds = (Rectangle){ 40.0f, 40.0f, (float)screenWidth - 80.0f, (float)screenHeight - 80.0f };
     m->wallCount = 0;
     m->stallCount = 0;
     m->lanternCount = 0;
     m->npcCount = 0;
     m->lanternsStrangeColors = false;
     m->stallShifted = false;
 
     BuildGeometry(m, screenWidth, screenHeight);
     BuildStalls(m);
     BuildLanterns(m);
     BuildNpcs(m);
 
     // Uncanny zone: a narrow side alley on the right
     m->uncannyZone = (Rectangle){ m->bounds.x + m->bounds.width - 120.0f, m->bounds.y + 140.0f, 80.0f, 160.0f };
 
     return m;
 }
 
 void Map_Destroy(Map *map) {
     if (map) free(map);
 }
 
 const Rectangle *Map_GetWalls(const Map *map, int *count) {
     if (count) *count = map->wallCount;
     return map->walls;
 }
 
 const Stall *Map_GetStalls(const Map *map, int *count) {
     if (count) *count = map->stallCount;
     return map->stalls;
 }
 
 const NpcSpot *Map_GetNpcs(const Map *map, int *count) {
     if (count) *count = map->npcCount;
     return map->npcs;
 }
 
 Rectangle Map_GetUncannyZone(const Map *map) {
     return map->uncannyZone;
 }
 
 static void BuildGeometry(Map *map, int screenWidth, int screenHeight) {
     // Outer walls
     float margin = 36.0f;
     Rectangle play = (Rectangle){ margin, margin, screenWidth - margin*2.0f, screenHeight - margin*2.0f };
 
     map->walls[map->wallCount++] = (Rectangle){ play.x, play.y, play.width, 12.0f };
     map->walls[map->wallCount++] = (Rectangle){ play.x, play.y + play.height - 12.0f, play.width, 12.0f };
     map->walls[map->wallCount++] = (Rectangle){ play.x, play.y, 12.0f, play.height };
     map->walls[map->wallCount++] = (Rectangle){ play.x + play.width - 12.0f, play.y, 12.0f, play.height };
 
     // Central walkway (visual only)
     (void)screenWidth;
     (void)screenHeight;
 
     // Some crates / carts as obstacles
     map->walls[map->wallCount++] = (Rectangle){ play.x + 150.0f, play.y + 220.0f, 40.0f, 40.0f };
     map->walls[map->wallCount++] = (Rectangle){ play.x + 260.0f, play.y + 360.0f, 60.0f, 36.0f };
     map->walls[map->wallCount++] = (Rectangle){ play.x + play.width - 220.0f, play.y + 260.0f, 60.0f, 40.0f };
 
     // Narrow alleys using walls
     map->walls[map->wallCount++] = (Rectangle){ play.x + play.width * 0.5f - 16.0f, play.y + 130.0f, 32.0f, 120.0f };
 }
 
 static void BuildStalls(Map *map) {
     float yTop = map->bounds.y + 80.0f;
     float yBottom = map->bounds.y + map->bounds.height - 180.0f;
     float xStart = map->bounds.x + 80.0f;
     float step = 140.0f;
 
     // Food stall
     map->stalls[map->stallCount++] = (Stall){
         (Rectangle){ xStart, yTop, 80.0f, 50.0f },
         STALL_FOOD,
         "Skewers"
     };
 
     // Mask stall (masked customer near this)
     map->stalls[map->stallCount++] = (Stall){
         (Rectangle){ xStart + step, yTop + 10.0f, 90.0f, 50.0f },
         STALL_MASKS,
         "Masks"
     };
 
     // Incense stall
     map->stalls[map->stallCount++] = (Stall){
         (Rectangle){ xStart + step * 2.0f, yTop + 5.0f, 100.0f, 55.0f },
         STALL_INCENSE,
         "Incense"
     };
 
     // Misc uncanny stall (shifts after anomaly)
     map->stalls[map->stallCount++] = (Stall){
         (Rectangle){ xStart + step * 0.5f, yBottom, 90.0f, 55.0f },
         STALL_MISC,
         "Lost & Found"
     };
 }
 
 static void BuildLanterns(Map *map) {
     float y = map->bounds.y + 60.0f;
     float xStart = map->bounds.x + 70.0f;
     float xEnd = map->bounds.x + map->bounds.width - 70.0f;
     float step = (xEnd - xStart) / 6.0f;
 
     for (int i = 0; i < 7 && i < MAX_LANTERNS; i++) {
         map->lanterns[map->lanternCount++] = (Lantern){
             (Vector2){ xStart + step * i, y },
             10.0f
         };
     }
 }
 
 static void BuildNpcs(Map *map) {
     // Suspicious food vendor near food stall
     Stall *food = &map->stalls[0];
     map->npcs[map->npcCount++] = (NpcSpot){
         (Rectangle){ food->bounds.x + food->bounds.width * 0.5f - 10.0f,
                      food->bounds.y + food->bounds.height + 8.0f,
                      20.0f, 24.0f },
         "Vendor"
     };
 
     // Quiet masked customer near mask stall
     Stall *mask = &map->stalls[1];
     map->npcs[map->npcCount++] = (NpcSpot){
         (Rectangle){ mask->bounds.x + mask->bounds.width + 10.0f,
                      mask->bounds.y + 4.0f,
                      18.0f, 24.0f },
         "Masked"
     };
 
     // Child voice location in uncanny alley (no visible body)
     map->npcs[map->npcCount++] = (NpcSpot){
         (Rectangle){ map->uncannyZone.x + map->uncannyZone.width * 0.5f - 8.0f,
                      map->uncannyZone.y + map->uncannyZone.height * 0.5f - 8.0f,
                      16.0f, 16.0f },
         "???"
     };
 
     // A generic passerby
     map->npcs[map->npcCount++] = (NpcSpot){
         (Rectangle){ map->bounds.x + 80.0f,
                      map->bounds.y + map->bounds.height * 0.5f,
                      18.0f, 24.0f },
         "Customer"
     };
 }
 
 void Map_DrawBackground(const Map *map) {
     // Night sky tint
     ClearBackground((Color){ 10, 7, 20, 255 });
 
     // Ground
     DrawRectangleRec(map->bounds, (Color){ 20, 20, 35, 255 });
 
     // Central walkway
     Rectangle walkway = {
         map->bounds.x + 70.0f,
         map->bounds.y + 120.0f,
         map->bounds.width - 140.0f,
         map->bounds.height - 220.0f
     };
     DrawRectangleRounded(walkway, 0.02f, 4, (Color){ 35, 32, 50, 255 });
 
     // Alleys (darker)
     DrawRectangleRec(map->uncannyZone, (Color){ 10, 5, 20, 255 });
 
     // Stalls
     for (int i = 0; i < map->stallCount; i++) {
         const Stall *s = &map->stalls[i];
         Color c = { 70, 40, 40, 255 };
         if (s->type == STALL_MASKS)      c = (Color){ 50, 40, 70, 255 };
         else if (s->type == STALL_INCENSE) c = (Color){ 60, 40, 50, 255 };
         else if (s->type == STALL_MISC)    c = (Color){ 40, 60, 60, 255 };
 
         DrawRectangleRounded(s->bounds, 0.1f, 6, c);
         DrawRectangleLinesEx(s->bounds, 2.0f, (Color){ 180, 160, 160, 255 });
 
         Vector2 textPos = { s->bounds.x + 6.0f, s->bounds.y - 18.0f };
         DrawText(s->label, (int)textPos.x, (int)textPos.y, 14, (Color){ 230, 220, 220, 255 });
     }
 
     // Npcs as silhouettes
     for (int i = 0; i < map->npcCount; i++) {
         const NpcSpot *n = &map->npcs[i];
         Color body = (Color){ 15, 10, 25, 255 };
         if (n->label && n->label[0] == 'M') {
             body = (Color){ 8, 8, 18, 255 };
         }
         DrawRectangleRounded(n->bounds, 0.3f, 6, body);
     }
 
     // Lanterns
     for (int i = 0; i < map->lanternCount; i++) {
         const Lantern *l = &map->lanterns[i];
         Color c = map->lanternsStrangeColors
                   ? (Color){ 150, 40, 190, 255 }
                   : (Color){ 255, 120, 80, 255 };
         DrawCircleV(l->position, l->radius + 3.0f, (Color){ c.r, c.g, c.b, 60 });
         DrawCircleV(l->position, l->radius, c);
     }
 }
 
 void Map_DrawForeground(const Map *map) {
     // Subtle vignette
     float w = map->bounds.width;
     float h = map->bounds.height;
     int steps = 8;
     for (int i = 0; i < steps; i++) {
         float t = (float)i / (float)steps;
         unsigned char alpha = (unsigned char)(40 + t * 60.0f);
         DrawRectangleLinesEx(
             (Rectangle){
                 map->bounds.x - t*10.0f,
                 map->bounds.y - t*6.0f,
                 w + t*20.0f,
                 h + t*12.0f
             },
             1.0f,
             (Color){ 0, 0, 0, alpha }
         );
     }
 }
