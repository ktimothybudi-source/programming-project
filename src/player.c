/*
 * ============================================================================
 * player.c — MOVING AND DRAWING THE PLAYER
 * ============================================================================
 *
 * Movement reads WASD and arrow keys. Two collision boxes:
 *   FULL BODY (GetBounds) — interaction (Press E)
 *   FEET (GetCollisionBounds) — small box at the bottom of the sprite — walls only
 *
 * Sprites: assets/player_left.png (move left / face left) and player_right.png (face right);
 * then placeholder rectangle if missing.
 *
 * Solid collision comes from the map's pink-mask grid (see assets/new_collision_map.png).
 * ============================================================================
 */

#include "player.h"
#include "map.h"
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static void MoveWithCollisions(Player *player, struct Map *map, Vector2 delta);
static void ResolveAxisX(Player *player, struct Map *map, float oldX, float deltaX);
static void ResolveAxisY(Player *player, struct Map *map, float oldY, float deltaY);
static void LoadPlayerSprites(Player *player);
static void DrawPlayerGroundShadow(const Player *player);

/* Wall collision: fraction of sprite size — small “feet” so tight spaces are easier. */
static const float COLL_BOX_W_FRAC = 0.30f;
static const float COLL_BOX_H_FRAC = 0.18f;

Player *Player_Create(void) {
    Player *p = (Player *)malloc(sizeof(Player));
    if (!p) return NULL;

    p->size = (Vector2){ 172.0f, 234.0f };
    p->position = (Vector2){ 400.0f, 400.0f };
    p->speed = 200.0f;
    p->color = (Color){ 240, 240, 255, 255 };
    p->spriteLeft = (Texture2D){ 0 };
    p->spriteRight = (Texture2D){ 0 };
    for (int i = 0; i < PLAYER_WALK_FRAMES; i++)
        p->walkFrames[i] = (Texture2D){ 0 };
    p->walkFrameCount = 0;
    p->animFrame = 0;
    p->animAccum = 0.0f;
    p->facing = 1;
    LoadPlayerSprites(p);
    return p;
}

void Player_Destroy(Player *player) {
    if (!player) return;
    for (int i = 0; i < PLAYER_WALK_FRAMES; i++) {
        if (player->walkFrames[i].id != 0)
            UnloadTexture(player->walkFrames[i]);
    }
    if (player->spriteLeft.id != 0) UnloadTexture(player->spriteLeft);
    if (player->spriteRight.id != 0) UnloadTexture(player->spriteRight);
    free(player);
}

Rectangle Player_GetBounds(const Player *player) {
    return (Rectangle){
        player->position.x - player->size.x * 0.5f,
        player->position.y - player->size.y * 0.5f,
        player->size.x,
        player->size.y
    };
}

Rectangle Player_GetCollisionBounds(const Player *player) {
    float w = player->size.x * COLL_BOX_W_FRAC;
    float h = player->size.y * COLL_BOX_H_FRAC;
    float bottom = player->position.y + player->size.y * 0.5f;
    return (Rectangle){
        player->position.x - w * 0.5f,
        bottom - h,
        w,
        h
    };
}

bool Player_IsNearPoint(const Player *player, Vector2 point, float range) {
    float dx = player->position.x - point.x;
    float dy = player->position.y - point.y;
    return (dx * dx + dy * dy) <= (range * range);
}

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

    if (player->walkFrameCount == PLAYER_WALK_FRAMES && len > 0.01f) {
        player->animAccum += dt * 9.0f;
        while (player->animAccum >= 1.0f) {
            player->animAccum -= 1.0f;
            player->animFrame = (player->animFrame + 1) % PLAYER_WALK_FRAMES;
        }
    } else if (player->walkFrameCount == PLAYER_WALK_FRAMES) {
        player->animFrame = 0;
        player->animAccum = 0.0f;
    }

    Vector2 delta = { input.x * player->speed * dt, input.y * player->speed * dt };
    MoveWithCollisions(player, map, delta);
}

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
    DrawPlayerGroundShadow(player);

    if (player->walkFrameCount == PLAYER_WALK_FRAMES) {
        Texture2D tex = player->walkFrames[player->animFrame];
        if (tex.id != 0) {
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            /* Walk PNGs face right; mirror when walking left (facing == 0). */
            if (player->facing == 0) {
                src.x = (float)tex.width;
                src.width = -(float)tex.width;
            }
            DrawTexturePro(tex, src, r, (Vector2){ 0, 0 }, 0.0f, WHITE);
            return;
        }
    }

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

static void DrawPlayerGroundShadow(const Player *player) {
    if (!player) return;
    float cx = player->position.x;
    float footY = player->position.y + player->size.y * 0.5f;
    float rx = fmaxf(10.0f, player->size.x * 0.44f);
    float ry = fmaxf(6.0f, player->size.x * 0.13f);
    DrawEllipse((int)cx, (int)(footY + ry * 0.35f), rx, ry, (Color){ 10, 8, 18, 82 });
}

static void LoadPlayerSprites(Player *player) {
    if (FileExists("assets/player_left.png")) {
        player->spriteLeft = LoadTexture("assets/player_left.png");
        if (player->spriteLeft.id != 0)
            SetTextureFilter(player->spriteLeft, TEXTURE_FILTER_POINT);
    }
    if (FileExists("assets/player_right.png")) {
        player->spriteRight = LoadTexture("assets/player_right.png");
        if (player->spriteRight.id != 0)
            SetTextureFilter(player->spriteRight, TEXTURE_FILTER_POINT);
    }
}
