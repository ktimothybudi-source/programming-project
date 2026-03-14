 #ifndef PLAYER_H
 #define PLAYER_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 struct Map;
 
 typedef struct Player {
     Vector2 position;
     Vector2 size;
     float speed;
     Color color;
 } Player;
 
 // Create / destroy
 Player *Player_Create(void);
 void    Player_Destroy(Player *player);
 
 // Update and draw
 void    Player_Update(Player *player, struct Map *map, float dt, bool canMove);
 void    Player_Draw(const Player *player);
 
 // Interaction helpers
 Rectangle Player_GetBounds(const Player *player);
 bool      Player_IsNearPoint(const Player *player, Vector2 point, float range);
 
 #endif // PLAYER_H
