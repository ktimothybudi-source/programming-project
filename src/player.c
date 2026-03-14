 #include "player.h"
 #include "map.h"
 #include "raylib.h"
 #include <stdlib.h>
 #include <math.h>
 
 static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta);
 
 Player *Player_Create(void) {
     Player *p = (Player *)malloc(sizeof(Player));
     if (!p) return NULL;
 
     p->size = (Vector2){ 18.0f, 24.0f };
     p->position = (Vector2){ 400.0f, 400.0f };
     p->speed = 140.0f;
     p->color = (Color){ 240, 240, 255, 255 };
     return p;
 }
 
 void Player_Destroy(Player *player) {
     if (player) free(player);
 }
 
 Rectangle Player_GetBounds(const Player *player) {
     return (Rectangle){
         player->position.x - player->size.x * 0.5f,
         player->position.y - player->size.y * 0.5f,
         player->size.x,
         player->size.y
     };
 }
 
 bool Player_IsNearPoint(const Player *player, Vector2 point, float range) {
     float dx = player->position.x - point.x;
     float dy = player->position.y - point.y;
     return (dx*dx + dy*dy) <= (range * range);
 }
 
 void Player_Update(Player *player, struct Map *map, float dt, bool canMove) {
     if (!canMove) return;
 
     Vector2 input = { 0.0f, 0.0f };
     if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    input.y -= 1.0f;
     if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  input.y += 1.0f;
     if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  input.x -= 1.0f;
     if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
 
     float len = sqrtf(input.x * input.x + input.y * input.y);
     if (len > 0.0f) {
         input.x /= len;
         input.y /= len;
     }
 
     Vector2 delta = { input.x * player->speed * dt, input.y * player->speed * dt };
     MoveWithCollisions(player, map, delta);
 }
 
 static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta) {
     int wallCount = 0;
     const Rectangle *walls = Map_GetWalls(map, &wallCount);
 
     // Horizontal
     player->position.x += delta.x;
     Rectangle bounds = Player_GetBounds(player);
     for (int i = 0; i < wallCount; i++) {
         if (CheckCollisionRecs(bounds, walls[i])) {
             if (delta.x > 0) {
                 player->position.x = walls[i].x - bounds.width * 0.5f;
             } else if (delta.x < 0) {
                 player->position.x = walls[i].x + walls[i].width + bounds.width * 0.5f;
             }
             bounds = Player_GetBounds(player);
         }
     }
 
     // Vertical
     player->position.y += delta.y;
     bounds = Player_GetBounds(player);
     for (int i = 0; i < wallCount; i++) {
         if (CheckCollisionRecs(bounds, walls[i])) {
             if (delta.y > 0) {
                 player->position.y = walls[i].y - bounds.height * 0.5f;
             } else if (delta.y < 0) {
                 player->position.y = walls[i].y + walls[i].height + bounds.height * 0.5f;
             }
             bounds = Player_GetBounds(player);
         }
     }
 }
 
 void Player_Draw(const Player *player) {
     Rectangle r = Player_GetBounds(player);
 
     DrawRectangleRounded(r, 0.3f, 6, player->color);
     DrawRectangleLinesEx(r, 2.0f, (Color){ 180, 180, 220, 255 });
 
     // Simple "face" indicator
     DrawCircle((int)player->position.x, (int)(player->position.y - player->size.y * 0.15f), 3.0f, (Color){ 30, 30, 50, 255 });
 }
