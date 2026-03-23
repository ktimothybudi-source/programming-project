/*
 * ============================================================================
 * player.h — DECLARATIONS FOR THE PLAYER CHARACTER
 * ============================================================================
 *
 * A ".h" file (header) tells the COMPILER what names exist so other .c files
 * can call player functions without knowing the full implementation.
 *
 * "struct Map" is only forward-declared here because player.c needs to know
 * the map exists for collision, but we do not include map.h in this header
 * to avoid messy circular includes.
 * ============================================================================
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

/* Forward declaration: "there is a struct called Map somewhere else." */
struct Map;

/*
 * Player stores everything we need to draw and move one character:
 * - position: center point in world coordinates (x, y)
 * - size: width and height of the rectangle we draw
 * - speed: how fast they move (pixels per second, used with dt)
 * - color: RGBA color for the simple rectangle art
 */
typedef struct Player {
    Vector2 position;
    Vector2 size;
    float speed;
    Color color;
} Player;

/* Create memory for a player and set starting values. */
Player *Player_Create(void);
/* Free memory when the game shuts down. */
void    Player_Destroy(Player *player);

/* Called every frame when the player is allowed to move. */
void    Player_Update(Player *player, struct Map *map, float dt, bool canMove);
/* Draw the player on screen (world space; camera is applied in game.c). */
void    Player_Draw(const Player *player);

/*
 * Rectangle = axis-aligned box: x, y, width, height.
 * Player_GetBounds = whole body box (used for "press E" overlap checks).
 * Player_GetCollisionBounds = lower half of sprite, full width (used for walls).
 */
Rectangle Player_GetBounds(const Player *player);
/* Code created by wu deguang - feet/lower-body collision for wall checks */
Rectangle Player_GetCollisionBounds(const Player *player);
/* True if player is within distance "range" of a point (Pythagorean distance). */
bool      Player_IsNearPoint(const Player *player, Vector2 point, float range);

#endif /* PLAYER_H */
