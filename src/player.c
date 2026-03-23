/*
 * ============================================================================
 * player.c — MOVING AND DRAWING THE PLAYER
 * ============================================================================
 *
 * The player is a simple colored rectangle (placeholder art). Movement reads
 * WASD and arrow keys. Two collision boxes:
   * FULL BODY (GetBounds) — interaction (Press E)
   * FEET (GetCollisionBounds) — lower half of sprite, full width — walls
 *
 * Solid collision comes from the map's red-mask grid (see assets/collision_mask.png).
 * ============================================================================
 */

#include "player.h"
#include "map.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta);
static void ResolveAxisX(Player *player, struct Map *map, float oldX, float deltaX);
static void ResolveAxisY(Player *player, struct Map *map, float oldY, float deltaY);
static void LoadPlayerSprites(Player *player);

Player *Player_Create(void) {
    /* malloc = ask the OS for memory; sizeof(Player) is how many bytes we need. */
    Player *p = (Player *)malloc(sizeof(Player));
    if (!p) return NULL;

    /* Significantly larger sprite footprint: 8x the original (was 22x30). */
    p->size = (Vector2){ 176.0f, 240.0f };
    /* Starting position (overwritten when a real day starts). */
    p->position = (Vector2){ 400.0f, 400.0f };
    /* How many pixels per second we move when a key is held (before dt scaling). */
    p->speed = 200.0f;
    p->color = (Color){ 240, 240, 255, 255 };
    p->spriteLeft = (Texture2D){ 0 };
    p->spriteRight = (Texture2D){ 0 };
    p->facing = 1;
    LoadPlayerSprites(p);
    return p;
}

void Player_Destroy(Player *player) {
    if (!player) return;
    if (player->spriteLeft.id != 0) UnloadTexture(player->spriteLeft);
    if (player->spriteRight.id != 0) UnloadTexture(player->spriteRight);
    free(player);
}

/*
 * Full-body rectangle: centered on position.
 * Example: if position is (100, 200) and size is 22x30, the top-left of the
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

/*
 * Lower-body collision: full sprite width (matches what you see), lower half of
 * sprite height, anchored from the character center downward.
 */
Rectangle Player_GetCollisionBounds(const Player *player) {
    float w = player->size.x;
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

    if (input.x < 0.0f) player->facing = 0;
    else if (input.x > 0.0f) player->facing = 1;

    float len = sqrtf(input.x * input.x + input.y * input.y);
    if (len > 0.0f) {
        input.x /= len;
        input.y /= len;
    }

    Vector2 delta = { input.x * player->speed * dt, input.y * player->speed * dt };
    MoveWithCollisions(player, map, delta);
}

/*
 * Axis-separated movement vs collision grid; binary search for max displacement when blocked.
 */
static void ResolveAxisX(Player *player, struct Map *map, float oldX, float deltaX) {
    if (deltaX == 0.0f) return;
    player->position.x += deltaX;
    if (!Map_RectBlocked(map, Player_GetCollisionBounds(player)))
        return;
    float t0 = 0.0f, t1 = 1.0f;
    player->position.x = oldX + deltaX * t1;
    if (!Map_RectBlocked(map, Player_GetCollisionBounds(player)))
        return;
    for (int i = 0; i < 24; i++) {
        float tm = (t0 + t1) * 0.5f;
        player->position.x = oldX + deltaX * tm;
        if (Map_RectBlocked(map, Player_GetCollisionBounds(player))) t1 = tm;
        else t0 = tm;
    }
    player->position.x = oldX + deltaX * t0;
}

static void ResolveAxisY(Player *player, struct Map *map, float oldY, float deltaY) {
    if (deltaY == 0.0f) return;
    player->position.y += deltaY;
    if (!Map_RectBlocked(map, Player_GetCollisionBounds(player)))
        return;
    float t0 = 0.0f, t1 = 1.0f;
    player->position.y = oldY + deltaY * t1;
    if (!Map_RectBlocked(map, Player_GetCollisionBounds(player)))
        return;
    for (int i = 0; i < 24; i++) {
        float tm = (t0 + t1) * 0.5f;
        player->position.y = oldY + deltaY * tm;
        if (Map_RectBlocked(map, Player_GetCollisionBounds(player))) t1 = tm;
        else t0 = tm;
    }
    player->position.y = oldY + deltaY * t0;
}

static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta) {
    float ox = player->position.x;
    float oy = player->position.y;
    ResolveAxisX(player, map, ox, delta.x);
    ResolveAxisY(player, map, oy, delta.y);
}

void Player_Draw(const Player *player) {
    Rectangle r = Player_GetBounds(player);
    Texture2D sprite = (Texture2D){ 0 };
    if (player->facing == 0) sprite = player->spriteLeft;
    else sprite = player->spriteRight;

    if (sprite.id != 0) {
        Rectangle src = (Rectangle){ 0, 0, (float)sprite.width, (float)sprite.height };
        DrawTexturePro(sprite, src, r, (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangleRounded(r, 0.3f, 6, player->color);
        DrawRectangleLinesEx(r, 2.0f, (Color){ 180, 180, 220, 255 });
        DrawCircle((int)player->position.x, (int)(player->position.y - player->size.y * 0.15f), 4.0f, (Color){ 30, 30, 50, 255 });
    }
}

static void LoadPlayerSprites(Player *player) {
    if (FileExists("assets/player_left.png")) player->spriteLeft = LoadTexture("assets/player_left.png");
    if (FileExists("assets/player_right.png")) player->spriteRight = LoadTexture("assets/player_right.png");
}
