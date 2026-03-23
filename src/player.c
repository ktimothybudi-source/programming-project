/*
 * ============================================================================
 * player.c — MOVING AND DRAWING THE PLAYER
 * ============================================================================
 *
 * The player is a simple colored rectangle (placeholder art). Movement reads
 * WASD and arrow keys. We split "collision" into two parts:
   * FULL BODY (GetBounds) — generous overlap with interaction zones
   * FEET ONLY (GetCollisionBounds) — narrow box so walls feel tight
 *
 * Walls come from the map as a list of rectangles. If the player overlaps a
 * wall after moving, we "push back" their position so they stay outside.
 * ============================================================================
 */

#include "player.h"
#include "map.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

/* Forward declaration: collision logic is private to this file. */
static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta);

Player *Player_Create(void) {
    /* malloc = ask the OS for memory; sizeof(Player) is how many bytes we need. */
    Player *p = (Player *)malloc(sizeof(Player));
    if (!p) return NULL;

    /* size.x = width, size.y = height of the drawn rectangle in world units. */
    p->size = (Vector2){ 18.0f, 24.0f };
    /* Starting position (overwritten when a real day starts). */
    p->position = (Vector2){ 400.0f, 400.0f };
    /* How many pixels per second we move when a key is held (before dt scaling). */
    p->speed = 140.0f;
    p->color = (Color){ 240, 240, 255, 255 };
    return p;
}

void Player_Destroy(Player *player) {
    if (player) free(player);
}

/*
 * Full-body rectangle: centered on position.
 * Example: if position is (100, 200) and size is 18x24, the top-left of the
 * rect is (91, 188) so the center stays at (100, 200).
 */
Rectangle Player_GetBounds(const Player *player) {
    return (Rectangle){
        player->position.x - player->size.x * 0.5f,
        player->position.y - player->size.y * 0.5f,
        player->size.x,
        player->size.y
    };
}

/* Code created by wu deguang */
/*
 * Feet / lower-body collision only — half the sprite width, half the height,
 * anchored from the character center downward. This is NOT the full sprite;
 * it stops the player from walking through shelves without needing a huge box.
 */
Rectangle Player_GetCollisionBounds(const Player *player) {
    float w = player->size.x * 0.5f;
    float h = player->size.y * 0.5f;
    return (Rectangle){
        player->position.x - w * 0.5f,
        player->position.y,
        w,
        h
    };
}

/*
 * Compare distance to point without sqrt (we compare squared distances to "range^2").
 * This is a tiny optimization and avoids floating sqrt for a simple check.
 */
bool Player_IsNearPoint(const Player *player, Vector2 point, float range) {
    float dx = player->position.x - point.x;
    float dy = player->position.y - point.y;
    return (dx*dx + dy*dy) <= (range * range);
}

/*
 * Read keyboard. Build a direction vector (input.x, input.y).
 * If both keys are pressed diagonally, we normalize so diagonal speed = same as cardinal.
 * "delta" = how far to move this frame = direction * speed * dt.
 */
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

/* Code created by wu deguang */
/*
 * Move along X first, fix overlaps with walls, then move along Y and fix again.
 * This is a simple approach; corners can occasionally feel sticky but it is easy to read.
 */
static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta) {
    int wallCount = 0;
    const Rectangle *walls = Map_GetWalls(map, &wallCount);
    Rectangle bounds = Player_GetCollisionBounds(player);

    /* --- Horizontal movement --- */
    player->position.x += delta.x;
    bounds = Player_GetCollisionBounds(player);
    for (int i = 0; i < wallCount; i++) {
        if (CheckCollisionRecs(bounds, walls[i])) {
            if (delta.x > 0) {
                /* Moving right into a wall: snap to the left side of the wall. */
                player->position.x = walls[i].x - bounds.width * 0.5f;
            } else if (delta.x < 0) {
                /* Moving left into a wall: snap to the right side of the wall. */
                player->position.x = walls[i].x + walls[i].width + bounds.width * 0.5f;
            }
            bounds = Player_GetCollisionBounds(player);
        }
    }

    /* --- Vertical movement --- */
    player->position.y += delta.y;
    bounds = Player_GetCollisionBounds(player);
    for (int i = 0; i < wallCount; i++) {
        if (CheckCollisionRecs(bounds, walls[i])) {
            if (delta.y > 0) {
                /* Moving down: bottom of feet hit wall top. */
                player->position.y = walls[i].y - bounds.height;
            } else if (delta.y < 0) {
                /* Moving up: top of feet hit wall bottom. */
                player->position.y = walls[i].y + walls[i].height;
            }
            bounds = Player_GetCollisionBounds(player);
        }
    }
}

void Player_Draw(const Player *player) {
    Rectangle r = Player_GetBounds(player);

    DrawRectangleRounded(r, 0.3f, 6, player->color);
    DrawRectangleLinesEx(r, 2.0f, (Color){ 180, 180, 220, 255 });

    /* Tiny dot so you can see which way is "up" on the placeholder. */
    DrawCircle((int)player->position.x, (int)(player->position.y - player->size.y * 0.15f), 3.0f, (Color){ 30, 30, 50, 255 });
}
