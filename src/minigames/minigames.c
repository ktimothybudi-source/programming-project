/*
 * Minigames adapted from mingame/*.c — single-window, virtual resolution, letterboxed.
 */
#include "minigames.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------- */
/* Host: render texture + mouse map                                           */
/* -------------------------------------------------------------------------- */

static MinigameId g_id = MINIGAME_NONE;
static RenderTexture2D g_rt;
static int g_vw = 800;
static int g_vh = 450;
static float g_scale;
static float g_ox;
static float g_oy;

static void host_layout(int screenW, int screenH) {
    g_scale = fminf((float)screenW / (float)g_vw, (float)screenH / (float)g_vh);
    g_ox = ((float)screenW - (float)g_vw * g_scale) * 0.5f;
    g_oy = ((float)screenH - (float)g_vh * g_scale) * 0.5f;
}

static Vector2 host_mouse_virtual(void) {
    Vector2 m = GetMousePosition();
    return (Vector2){ (m.x - g_ox) / g_scale, (m.y - g_oy) / g_scale };
}

static void host_begin_texture(void) {
    BeginTextureMode(g_rt);
    ClearBackground((Color){ 15, 12, 22, 255 });
}

static void host_end_texture_draw(int screenW, int screenH) {
    EndTextureMode();
    host_layout(screenW, screenH);
    Texture2D tex = g_rt.texture;
    DrawTexturePro(
        tex,
        (Rectangle){ 0.0f, 0.0f, (float)tex.width, -(float)tex.height },
        (Rectangle){ g_ox, g_oy, (float)g_vw * g_scale, (float)g_vh * g_scale },
        (Vector2){ 0.0f, 0.0f },
        0.0f,
        WHITE);
}

static void host_set_size(int w, int h) {
    if (g_rt.texture.id != 0) {
        UnloadRenderTexture(g_rt);
        g_rt = (RenderTexture2D){ 0 };
    }
    g_vw = w;
    g_vh = h;
    g_rt = LoadRenderTexture(w, h);
    SetTextureFilter(g_rt.texture, TEXTURE_FILTER_BILINEAR);
}

/* -------------------------------------------------------------------------- */
/* Card swipe                                                                 */
/* -------------------------------------------------------------------------- */
typedef enum { CS_SETUP, CS_DOCKED, CS_SWIPING, CS_SUCCESS } CsStage;
typedef enum { CS_FB_NONE, CS_FB_FAST, CS_FB_SLOW } CsFeedback;

static CsStage cs_stage;
static CsFeedback cs_fb;
static float cs_swipeStart;
static bool cs_drag;
static Vector2 cs_dragOff;
static Rectangle cs_readerBody;
static Rectangle cs_readerSlot;
static Rectangle cs_frontCover;
static Rectangle cs_card;

static void cardswipe_reset(void) {
    cs_readerBody = (Rectangle){ 100, 120, 600, 220 };
    cs_readerSlot = (Rectangle){ 120, 230, 560, 40 };
    cs_frontCover = (Rectangle){ 120, 200, 560, 45 };
    cs_card = (Rectangle){ 120, 360, 120, 80 };
    cs_stage = CS_SETUP;
    cs_fb = CS_FB_NONE;
    cs_drag = false;
}

static MinigameResult cardswipe_update(Vector2 mpos) {
    if (cs_stage == CS_SUCCESS) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
            return MINIGAME_WON;
        return MINIGAME_CONTINUE;
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mpos, cs_card)) {
        cs_drag = true;
        cs_dragOff.x = mpos.x - cs_card.x;
        cs_dragOff.y = mpos.y - cs_card.y;
        if (cs_stage == CS_DOCKED) {
            cs_swipeStart = (float)GetTime();
            cs_stage = CS_SWIPING;
        }
    }
    if (cs_drag) {
        if (cs_stage == CS_SETUP) {
            cs_card.x = mpos.x - cs_dragOff.x;
            cs_card.y = mpos.y - cs_dragOff.y;
            if (CheckCollisionRecs(cs_card, cs_readerSlot)) {
                cs_stage = CS_DOCKED;
                cs_drag = false;
                cs_card.y = 210;
                cs_card.x = 120;
            }
        } else if (cs_stage == CS_SWIPING) {
            cs_card.x = mpos.x - cs_dragOff.x;
            if (cs_card.x < 120) cs_card.x = 120;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                cs_drag = false;
                float duration = (float)GetTime() - cs_swipeStart;
                if (cs_card.x > 500) {
                    if (duration < 0.2f) {
                        cs_fb = CS_FB_FAST;
                        cs_stage = CS_DOCKED;
                        cs_card.x = 120;
                    } else if (duration > 1.2f) {
                        cs_fb = CS_FB_SLOW;
                        cs_stage = CS_DOCKED;
                        cs_card.x = 120;
                    } else
                        cs_stage = CS_SUCCESS;
                } else {
                    cs_card.x = 120;
                    cs_stage = CS_DOCKED;
                }
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) cs_drag = false;
    }
    return MINIGAME_CONTINUE;
}

static void cardswipe_draw(void) {
    DrawRectangleRec(cs_readerBody, (Color){ 60, 60, 65, 255 });
    DrawRectangleLinesEx(cs_readerBody, 3, (Color){ 80, 80, 85, 255 });
    DrawText("ID CARD READER v2", 310, 140, 20, LIGHTGRAY);
    DrawRectangleRec(cs_readerSlot, BLACK);
    BeginScissorMode(100, 120, 600, 300);
    DrawRectangleRec(cs_card, SKYBLUE);
    DrawRectangle((int)(cs_card.x + 10), (int)(cs_card.y + 10), 35, 35, RAYWHITE);
    DrawRectangle((int)(cs_card.x + 55), (int)(cs_card.y + 15), 50, 4, DARKGRAY);
    DrawRectangle((int)cs_card.x, (int)(cs_card.y + 60), (int)cs_card.width, 15, BLACK);
    EndScissorMode();
    DrawRectangleRec(cs_frontCover, (Color){ 70, 70, 75, 255 });
    DrawRectangleLinesEx(cs_frontCover, 2, (Color){ 90, 90, 95, 255 });
    DrawText("INSERT AND SWIPE", 320, 215, 18, RAYWHITE);
    Color light = RED;
    if (cs_stage == CS_DOCKED || cs_stage == CS_SWIPING) light = YELLOW;
    if (cs_stage == CS_SUCCESS) light = LIME;
    DrawCircle(650, 222, 10, light);
    DrawCircleGradient(650, 222, 5, Fade(WHITE, 0.5f), Fade(light, 0.0f));
    if (cs_stage == CS_SETUP) DrawText("Move card to slot...", 120, 410, 20, GRAY);
    if (cs_fb == CS_FB_FAST) DrawText("TOO FAST!", 350, 360, 30, RED);
    if (cs_fb == CS_FB_SLOW) DrawText("TOO SLOW!", 350, 360, 30, ORANGE);
    if (cs_stage == CS_SUCCESS) {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(BLACK, 0.7f));
        DrawText("ACCESS GRANTED", 240, 200, 40, LIME);
        DrawText("Click or Space to continue", 260, 260, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Mop                                                                        */
/* -------------------------------------------------------------------------- */
#define MOP_MAX 1500
#define MOP_W 60
#define MOP_H 30

typedef struct { Vector2 p; float r; bool on; } MopDrop;

static MopDrop mop_p[MOP_MAX];
static int mop_rem;
static bool mop_done;

static void DrawMopHead(Vector2 pos, bool scrub) {
    float tilt = scrub ? 12.0f : 0.0f;
    DrawRectanglePro((Rectangle){ pos.x, pos.y - 20, 8, 120 }, (Vector2){ 4, 120 }, 15 + tilt, BROWN);
    Rectangle head = { pos.x, pos.y, MOP_W, MOP_H };
    Vector2 origin = { MOP_W / 2.0f, MOP_H / 2.0f };
    DrawRectanglePro(head, origin, tilt, LIGHTGRAY);
    for (int i = -25; i <= 25; i += 8)
        DrawLineEx((Vector2){ pos.x + i, pos.y - 8 }, (Vector2){ pos.x + i, pos.y + 12 }, 2, GRAY);
}

static void mop_reset(void) {
    mop_rem = MOP_MAX;
    mop_done = false;
    int per = MOP_MAX / 5;
    for (int s = 0; s < 5; s++) {
        Vector2 c = { (float)GetRandomValue(200, 600), (float)GetRandomValue(200, 400) };
        for (int i = 0; i < per; i++) {
            int ix = s * per + i;
            float ang = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            float dist = (float)GetRandomValue(0, 70) + (float)GetRandomValue(0, 50);
            mop_p[ix].p.x = c.x + cosf(ang) * dist;
            mop_p[ix].p.y = c.y + sinf(ang) * dist;
            mop_p[ix].r = (float)GetRandomValue(5, 12);
            mop_p[ix].on = true;
        }
    }
}

static MinigameResult mop_update(Vector2 mpos) {
    if (mop_done) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return MINIGAME_WON;
        return MINIGAME_CONTINUE;
    }
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Rectangle mopRect = { mpos.x - MOP_W / 2.0f, mpos.y - MOP_H / 2.0f, (float)MOP_W, (float)MOP_H };
        for (int i = 0; i < MOP_MAX; i++) {
            if (mop_p[i].on && CheckCollisionCircleRec(mop_p[i].p, mop_p[i].r, mopRect)) {
                mop_p[i].on = false;
                mop_rem--;
            }
        }
    }
    if (mop_rem <= 0) {
        mop_rem = 0;
        mop_done = true;
    }
    return MINIGAME_CONTINUE;
}

static void mop_draw(Vector2 mpos) {
    for (int x = 0; x < g_vw; x += 60) DrawLine(x, 0, x, g_vh, Fade(BROWN, 0.1f));
    for (int y = 0; y < g_vh; y += 60) DrawLine(0, y, g_vw, y, Fade(BROWN, 0.1f));
    for (int i = 0; i < MOP_MAX; i++)
        if (mop_p[i].on) DrawCircleV(mop_p[i].p, mop_p[i].r, (Color){ 80, 140, 230, 180 });
    if (!mop_done) {
        DrawText("HOLD LEFT CLICK TO MOP", 20, 20, 20, DARKGRAY);
        float prog = (float)(MOP_MAX - mop_rem) / (float)MOP_MAX;
        DrawRectangle(20, 50, 250, 20, LIGHTGRAY);
        DrawRectangle(20, 50, (int)(250 * prog), 20, BLUE);
        DrawRectangleLines(20, 50, 250, 20, DARKGRAY);
        DrawText(TextFormat("%i%% Clean", (int)(prog * 100)), 280, 50, 20, DARKBLUE);
        DrawMopHead(mpos, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
    } else {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(RAYWHITE, 0.8f));
        DrawText("100% CLEAN - GREAT JOB!", g_vw / 2 - 200, g_vh / 2 - 20, 35, DARKGREEN);
        DrawText("Click / Space to continue", g_vw / 2 - 120, g_vh / 2 + 40, 20, DARKGRAY);
    }
}

/* -------------------------------------------------------------------------- */
/* Dishes (from dishes.c)                                                     */
/* -------------------------------------------------------------------------- */
#define DISHES_TOTAL 3
#define DISHES_DIRT 15

typedef enum { DISH_PILE, DISH_RINSING, DISH_DRYING } DishPlateState;

typedef struct {
    Vector2 offset;
    bool active;
} DishGrime;

typedef struct {
    Vector2 pos;
    float radius;
    float rinseProgress;
    DishPlateState state;
    DishGrime grime[DISHES_DIRT];
} DishPlate;

static Rectangle dishes_soapy;
static Rectangle dishes_faucet;
static Rectangle dishes_rack;
static DishPlate dishes_plates[DISHES_TOTAL];
static int dishes_finished;
static int dishes_active;

static void dishes_reset(void) {
    dishes_soapy = (Rectangle){ 50, 250, 220, 150 };
    dishes_faucet = (Rectangle){ 350, 150, 100, 200 };
    dishes_rack = (Rectangle){ 550, 250, 200, 150 };
    dishes_finished = 0;
    dishes_active = -1;
    for (int i = 0; i < DISHES_TOTAL; i++) {
        dishes_plates[i].pos = (Vector2){ 160.0f, 320.0f - (i * 5.0f) };
        dishes_plates[i].radius = 50.0f;
        dishes_plates[i].rinseProgress = 0.0f;
        dishes_plates[i].state = DISH_PILE;
        for (int j = 0; j < DISHES_DIRT; j++) {
            float r = (float)GetRandomValue(0, 35);
            float angle = (float)GetRandomValue(0, 360) * (PI / 180.0f);
            dishes_plates[i].grime[j].offset = (Vector2){ cosf(angle) * r, sinf(angle) * r };
            dishes_plates[i].grime[j].active = true;
        }
    }
}

static MinigameResult dishes_update(Vector2 mousePos) {
    if (dishes_finished >= DISHES_TOTAL) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return MINIGAME_WON;
        return MINIGAME_CONTINUE;
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && dishes_active == -1) {
        for (int i = DISHES_TOTAL - 1; i >= 0; i--) {
            if (dishes_plates[i].state == DISH_PILE
                && CheckCollisionPointCircle(mousePos, dishes_plates[i].pos, dishes_plates[i].radius)) {
                dishes_active = i;
                break;
            }
        }
    }
    if (dishes_active != -1) {
        dishes_plates[dishes_active].pos = mousePos;
        if (CheckCollisionCircleRec(dishes_plates[dishes_active].pos, dishes_plates[dishes_active].radius,
                                   dishes_faucet)) {
            dishes_plates[dishes_active].rinseProgress += 0.005f;
            for (int j = 0; j < DISHES_DIRT; j++) {
                if (dishes_plates[dishes_active].rinseProgress > (float)j / DISHES_DIRT)
                    dishes_plates[dishes_active].grime[j].active = false;
            }
            if (dishes_plates[dishes_active].rinseProgress >= 1.0f)
                dishes_plates[dishes_active].state = DISH_RINSING;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            DishPlate *held = &dishes_plates[dishes_active];
            if (held->state == DISH_RINSING
                && CheckCollisionCircleRec(held->pos, held->radius, dishes_rack)) {
                held->state = DISH_DRYING;
                held->pos = (Vector2){ 600.0f + (dishes_finished * 25.0f), 320.0f };
                dishes_finished++;
            } else {
                held->pos = (Vector2){ 160.0f, 320.0f };
                if (held->state == DISH_RINSING) {
                    held->state = DISH_PILE;
                    held->rinseProgress = 0.0f;
                    for (int j = 0; j < DISHES_DIRT; j++)
                        held->grime[j].active = true;
                }
            }
            dishes_active = -1;
        }
    }
    return MINIGAME_CONTINUE;
}

static void dishes_draw(void) {
    ClearBackground((Color){ 45, 55, 70, 255 });
    DrawRectangleRec(dishes_soapy, (Color){ 0, 121, 241, 50 });
    DrawRectangleLinesEx(dishes_soapy, 3, SKYBLUE);
    DrawText("DIRTY PILE", dishes_soapy.x + 60, dishes_soapy.y + 110, 20, SKYBLUE);
    DrawRectangleRec(dishes_faucet, (Color){ 0, 255, 255, 50 });
    for (int i = 0; i < 5; i++)
        DrawRectangle(dishes_faucet.x + 10 + (i * 20), dishes_faucet.y, 5, dishes_faucet.height, (Color){ 0, 255, 255, 80 });
    DrawRectangle(dishes_faucet.x - 10, dishes_faucet.y - 40, dishes_faucet.width + 20, 40, DARKGRAY);
    DrawText("RINSE", dishes_faucet.x + 30, dishes_faucet.y + 110, 20, RAYWHITE);
    DrawRectangleRec(dishes_rack, (Color){ 80, 80, 80, 255 });
    DrawRectangleLinesEx(dishes_rack, 4, BLACK);
    for (int i = 0; i < DISHES_TOTAL + 1; i++)
        DrawRectangleLinesEx((Rectangle){ dishes_rack.x + 40 + (i * 30), dishes_rack.y + 20, 2, dishes_rack.height - 40 }, 1, BLACK);
    DrawText("DISH RACK", dishes_rack.x + 60, dishes_rack.y + 110, 20, RAYWHITE);
    for (int i = 0; i < DISHES_TOTAL; i++) {
        DrawCircleV(dishes_plates[i].pos, dishes_plates[i].radius + 3, Fade(BLACK, 0.2f));
        DrawCircleV(dishes_plates[i].pos, dishes_plates[i].radius, RAYWHITE);
        DrawCircleLines(dishes_plates[i].pos.x, dishes_plates[i].pos.y, dishes_plates[i].radius, LIGHTGRAY);
        DrawCircleLines(dishes_plates[i].pos.x, dishes_plates[i].pos.y, dishes_plates[i].radius - 12, (Color){ 160, 160, 160, 255 });
        DrawCircleLines(dishes_plates[i].pos.x, dishes_plates[i].pos.y, dishes_plates[i].radius - 14, Fade(LIGHTGRAY, 0.4f));
        for (int j = 0; j < DISHES_DIRT; j++) {
            if (dishes_plates[i].grime[j].active) {
                Vector2 sPos = { dishes_plates[i].pos.x + dishes_plates[i].grime[j].offset.x,
                                 dishes_plates[i].pos.y + dishes_plates[i].grime[j].offset.y };
                DrawCircleV(sPos, 4, (Color){ 101, 78, 55, 255 });
            }
        }
    }
    DrawText(TextFormat("Dishes Cleaned: %d/%d", dishes_finished, DISHES_TOTAL), 10, 10, 20, RAYWHITE);
    DrawText("Drag the plate through the water!", 10, 35, 18, GRAY);
    if (dishes_finished >= DISHES_TOTAL) {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(BLACK, 0.7f));
        DrawText("ALL CLEAN!", g_vw / 2 - 100, g_vh / 2 - 30, 40, LIME);
        DrawText("Click / Space / Enter to continue", g_vw / 2 - 160, g_vh / 2 + 30, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Radio (slider tuner — from radio2.c)                                       */
/* -------------------------------------------------------------------------- */
typedef enum { RT_TUNING, RT_SUCCESS } Radio2State;

static Radio2State rt_state;
static float rt_target;
static float rt_cur;
static float rt_tol;
static Rectangle rt_dialBar;
static Rectangle rt_knob;

static void radio_reset(void) {
    rt_target = 95.5f;
    rt_cur = 88.0f;
    rt_tol = 0.5f;
    rt_state = RT_TUNING;
    rt_dialBar = (Rectangle){ 150, 320, 500, 40 };
    rt_knob = (Rectangle){ 150, 310, 20, 60 };
}

static MinigameResult radio_update(Vector2 mpos) {
    if (rt_state == RT_TUNING) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mpos, rt_dialBar)) {
            rt_knob.x = mpos.x - rt_knob.width / 2;
            if (rt_knob.x < 150) rt_knob.x = 150;
            if (rt_knob.x > 630) rt_knob.x = 630;
            rt_cur = 88.0f + ((rt_knob.x - 150) / 480.0f) * 20.0f;
        }
        if (fabsf(rt_cur - rt_target) < rt_tol && !IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            rt_state = RT_SUCCESS;
        return MINIGAME_CONTINUE;
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
        return MINIGAME_WON;
    return MINIGAME_CONTINUE;
}

static void radio_draw(void) {
    DrawRectangle(100, 50, 600, 350, (Color){ 60, 50, 40, 255 });
    DrawRectangle(120, 70, 560, 230, BLACK);
    float dist = fabsf(rt_cur - rt_target);
    Color wave = (dist < rt_tol) ? LIME : RED;
    for (int i = 0; i < 560; i++) {
        float noise = (dist > rt_tol) ? (float)(GetRandomValue(-(int)(dist * 5), (int)(dist * 5))) : 0;
        float y = 185 + sinf((i + GetTime() * 10) * 0.1f) * 40 + noise;
        DrawPixel(120 + i, (int)y, wave);
        DrawPixel(120 + i, (int)y + 1, wave);
    }
    DrawText(TextFormat("%.1f MHz", rt_cur), 320, 100, 40, wave);
    if (dist < 2.0f && rt_state == RT_TUNING) DrawText("SIGNAL DETECTED...", 330, 250, 15, YELLOW);
    DrawRectangleRec(rt_dialBar, DARKGRAY);
    DrawRectangleLinesEx(rt_dialBar, 2, LIGHTGRAY);
    for (int i = 0; i <= 10; i++) DrawRectangle(150 + i * 48, 320, 2, 10, WHITE);
    DrawRectangleRec(rt_knob, MAROON);
    DrawRectangleLinesEx(rt_knob, 2, GOLD);
    if (rt_state == RT_SUCCESS) {
        DrawRectangle(120, 70, 560, 230, Fade(BLACK, 0.8f));
        DrawText("STATION TUNED", 260, 160, 40, LIME);
        DrawText("Click to continue", 330, 220, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Trash                                                                      */
/* -------------------------------------------------------------------------- */
typedef enum { TB_DRAG, TB_STUFF, TB_OK } TbState;

static TbState tb_state;
static Rectangle tb_bag;
static Rectangle tb_binBody;
static Rectangle tb_binOpen;
static bool tb_isDrag;
static Vector2 tb_off;
static float tb_opac;

static void trash_reset(void) {
    tb_bag = (Rectangle){ 100, 300, 80, 100 };
    tb_binBody = (Rectangle){ 550, 200, 150, 200 };
    tb_binOpen = (Rectangle){ 550, 180, 150, 30 };
    tb_state = TB_DRAG;
    tb_isDrag = false;
    tb_opac = 1.0f;
}

static MinigameResult trash_update(Vector2 mpos) {
    if (tb_state == TB_OK) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
            return MINIGAME_WON;
        return MINIGAME_CONTINUE;
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mpos, tb_bag)) {
        tb_isDrag = true;
        tb_off.x = mpos.x - tb_bag.x;
        tb_off.y = mpos.y - tb_bag.y;
    }
    if (tb_isDrag) {
        tb_bag.x = mpos.x - tb_off.x;
        tb_bag.y = mpos.y - tb_off.y;
        if (CheckCollisionRecs(tb_bag, tb_binOpen)) {
            float bc = tb_bag.x + tb_bag.width / 2;
            float oc = tb_binOpen.x + tb_binOpen.width / 2;
            if (fabsf(bc - oc) < 30.0f)
                tb_state = TB_STUFF;
            else {
                if (tb_bag.x < tb_binOpen.x) tb_bag.x = tb_binOpen.x - 10;
                if (tb_bag.x + tb_bag.width > tb_binOpen.x + tb_binOpen.width)
                    tb_bag.x = tb_binOpen.x + tb_binOpen.width - tb_bag.width + 10;
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) tb_isDrag = false;
    }
    if (tb_state == TB_STUFF) {
        tb_isDrag = false;
        tb_bag.y += 5.0f;
        tb_opac -= 0.05f;
        if (tb_opac <= 0) tb_state = TB_OK;
    }
    return MINIGAME_CONTINUE;
}

static void trash_draw(void) {
    DrawRectangleRec(tb_binBody, DARKGRAY);
    DrawRectangle(550, 180, 150, 20, BLACK);
    DrawRectangleRounded(tb_bag, 0.4f, 8, Fade(DARKBLUE, tb_opac));
    DrawCircle((int)(tb_bag.x + 40), (int)tb_bag.y, 10, Fade(YELLOW, tb_opac));
    DrawRectangle(540, 200, 170, 15, GRAY);
    DrawRectangleLinesEx(tb_binBody, 3, BLACK);
    DrawText("DISPOSE OF TRASH", 545, 410, 18, LIGHTGRAY);
    if (tb_state == TB_DRAG && !tb_isDrag) DrawText("Drag the bag to the bin!", 80, 270, 20, RAYWHITE);
    if (tb_state == TB_STUFF) DrawText("STUFFING...", 580, 150, 20, YELLOW);
    if (tb_state == TB_OK) {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(BLACK, 0.7f));
        DrawText("TRASH DISPOSED", 240, 200, 40, LIME);
        DrawText("Click to continue", 310, 260, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Freezer                                                                    */
/* -------------------------------------------------------------------------- */
#define FZ_N 6

typedef struct {
    Rectangle r;
    Color col;
    bool rotten;
    bool grab;
    bool gone;
} FzMeat;

static FzMeat fz_m[FZ_N];
static int fz_rot;

static void freezer_reset(void) {
    fz_rot = 0;
    Rectangle fz = { 50, 50, 400, 350 };
    (void)fz;
    for (int i = 0; i < FZ_N; i++) {
        fz_m[i].r = (Rectangle){ (float)(80 + GetRandomValue(0, 300)), (float)(80 + GetRandomValue(0, 250)), 60, 40 };
        fz_m[i].grab = false;
        fz_m[i].gone = false;
        if (GetRandomValue(0, 1) == 0) {
            fz_m[i].rotten = true;
            fz_m[i].col = (Color){ 115, 130, 80, 255 };
            fz_rot++;
        } else {
            fz_m[i].rotten = false;
            fz_m[i].col = (Color){ 255, 160, 160, 255 };
        }
    }
}

static MinigameResult freezer_update(Vector2 mpos) {
    if (fz_rot <= 0) {
        if (IsKeyPressed(KEY_ESCAPE)) { /* allow ESC only after done? keep for cancel at top */ }
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            return MINIGAME_WON;
        return MINIGAME_CONTINUE;
    }
    for (int i = 0; i < FZ_N; i++) {
        if (fz_m[i].gone) continue;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mpos, fz_m[i].r)) {
            bool other = false;
            for (int j = 0; j < FZ_N; j++)
                if (fz_m[j].grab) other = true;
            if (!other) fz_m[i].grab = true;
        }
        if (fz_m[i].grab) {
            fz_m[i].r.x = mpos.x - fz_m[i].r.width / 2;
            fz_m[i].r.y = mpos.y - fz_m[i].r.height / 2;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                fz_m[i].grab = false;
                Rectangle trashCan = { 550, 250, 150, 150 };
                if (CheckCollisionRecs(fz_m[i].r, trashCan)) {
                    if (fz_m[i].rotten) {
                        fz_m[i].gone = true;
                        fz_rot--;
                    } else {
                        fz_m[i].r.x = (float)(100 + GetRandomValue(0, 200));
                        fz_m[i].r.y = (float)(100 + GetRandomValue(0, 200));
                    }
                }
            }
        }
    }
    return MINIGAME_CONTINUE;
}

static void freezer_draw(void) {
    Rectangle freezer = { 50, 50, 400, 350 };
    Rectangle trashCan = { 550, 250, 150, 150 };
    DrawRectangleRec(freezer, (Color){ 200, 230, 255, 255 });
    DrawRectangleLinesEx(freezer, 5, LIGHTGRAY);
    DrawText("FREEZER", 180, 20, 20, RAYWHITE);
    DrawRectangleRec(trashCan, DARKGRAY);
    DrawRectangleLinesEx(trashCan, 3, BLACK);
    DrawText("TRASH", 595, 220, 20, RED);
    for (int i = 0; i < FZ_N; i++) {
        if (!fz_m[i].gone) {
            DrawRectangleRec(fz_m[i].r, fz_m[i].col);
            DrawRectangleLinesEx(fz_m[i].r, 2, Fade(BLACK, 0.3f));
            if (fz_m[i].rotten) {
                DrawCircle((int)(fz_m[i].r.x + 15), (int)(fz_m[i].r.y + 10), 5, DARKBROWN);
                DrawCircle((int)(fz_m[i].r.x + 40), (int)(fz_m[i].r.y + 25), 3, DARKBROWN);
            }
        }
    }
    if (fz_rot > 0) {
        DrawText(TextFormat("Rotten meat left: %d", fz_rot), 500, 50, 20, RAYWHITE);
        DrawText("Drag GREEN meat to trash only!", 500, 80, 16, GRAY);
    } else {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(BLACK, 0.6f));
        DrawText("FREEZER CLEAN!", 250, 200, 40, LIME);
        DrawText("Click to continue", 300, 260, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Generator wires                                                            */
/* -------------------------------------------------------------------------- */
#define GW_N 4

typedef enum { GW_R = 0, GW_B, GW_G, GW_Y } GwCol;

typedef struct {
    Rectangle r;
    GwCol c;
    Color vis;
    int conn;
} GwL;

typedef struct {
    Rectangle r;
    GwCol c;
    Color vis;
    bool ok;
} GwR;

static GwL gw_L[GW_N];
static GwR gw_R[GW_N];
static int gw_sel;
static int gw_tot;
static bool gw_done;
static bool gw_active;

static void gen_reset(void) {
    gw_sel = -1;
    gw_tot = 0;
    gw_done = false;
    gw_active = true;
    int wh = 40, ww = 60;
    int sp = (g_vh - wh * GW_N) / (GW_N + 1);
    Color cols[4] = { RED, BLUE, GREEN, YELLOW };
    for (int i = 0; i < GW_N; i++) {
        gw_L[i].r = (Rectangle){ 50, (float)(sp + i * (wh + sp)), (float)ww, (float)wh };
        gw_L[i].c = (GwCol)i;
        gw_L[i].vis = cols[i];
        gw_L[i].conn = -1;
    }
    GwCol sh[GW_N] = { GW_G, GW_R, GW_Y, GW_B };
    for (int i = 0; i < GW_N; i++) {
        gw_R[i].r = (Rectangle){ (float)(g_vw - 50 - ww), (float)(sp + i * (wh + sp)), (float)ww, (float)wh };
        gw_R[i].c = sh[i];
        gw_R[i].vis = cols[sh[i]];
        gw_R[i].ok = false;
    }
}

static MinigameResult gen_update(Vector2 mpos) {
    if (!gw_active) return MINIGAME_CONTINUE;
    if (!gw_done) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int i = 0; i < GW_N; i++)
                if (CheckCollisionPointRec(mpos, gw_L[i].r)) {
                    gw_sel = i;
                    break;
                }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && gw_sel >= 0) {
            for (int j = 0; j < GW_N; j++) {
                if (CheckCollisionPointRec(mpos, gw_R[j].r)) {
                    if (gw_L[gw_sel].c == gw_R[j].c) {
                        gw_L[gw_sel].conn = j;
                        gw_R[j].ok = true;
                        gw_tot++;
                    }
                    break;
                }
            }
            gw_sel = -1;
        }
        if (gw_tot == GW_N) gw_done = true;
        return MINIGAME_CONTINUE;
    }
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return MINIGAME_WON;
    return MINIGAME_CONTINUE;
}

static void gen_draw(Vector2 mpos) {
    ClearBackground(DARKGRAY);
    for (int i = 0; i < GW_N; i++) {
        DrawRectangleRec(gw_L[i].r, gw_L[i].vis);
        if (gw_L[i].conn >= 0) {
            int d = gw_L[i].conn;
            Vector2 s = { gw_L[i].r.x + 60, gw_L[i].r.y + 20 };
            Vector2 e = { gw_R[d].r.x, gw_R[d].r.y + 20 };
            DrawLineEx(s, e, 8, gw_L[i].vis);
        }
    }
    for (int j = 0; j < GW_N; j++) DrawRectangleRec(gw_R[j].r, gw_R[j].vis);
    if (gw_sel >= 0) {
        Vector2 s = { gw_L[gw_sel].r.x + 60, gw_L[gw_sel].r.y + 20 };
        DrawLineEx(s, mpos, 8, gw_L[gw_sel].vis);
    }
    if (gw_done) {
        DrawRectangle(0, 0, g_vw, g_vh, Fade(BLACK, 0.5f));
        DrawText("GENERATOR ONLINE", 220, 200, 40, LIME);
        DrawText("Space / Click to continue", 250, 260, 20, RAYWHITE);
    }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

bool Minigames_Init(void) {
    g_id = MINIGAME_NONE;
    g_rt = (RenderTexture2D){ 0 };
    return true;
}

void Minigames_Shutdown(void) {
    if (g_rt.texture.id != 0) UnloadRenderTexture(g_rt);
    g_rt = (RenderTexture2D){ 0 };
    g_id = MINIGAME_NONE;
}

void Minigames_Start(MinigameId id) {
    g_id = id;
    if (id == MINIGAME_MOP)
        host_set_size(800, 600);
    else
        host_set_size(800, 450);
    srand((unsigned)time(NULL));
    switch (id) {
    case MINIGAME_CARD_SWIPE: cardswipe_reset(); break;
    case MINIGAME_MOP: mop_reset(); break;
    case MINIGAME_DISHES: dishes_reset(); break;
    case MINIGAME_RADIO: radio_reset(); break;
    case MINIGAME_TRASH: trash_reset(); break;
    case MINIGAME_FREEZER: freezer_reset(); break;
    case MINIGAME_GENERATOR: gen_reset(); break;
    default: break;
    }
}

MinigameId Minigames_GetActive(void) { return g_id; }

void Minigames_Clear(void) {
    g_id = MINIGAME_NONE;
}

MinigameResult Minigames_Update(float dt, int screenW, int screenH) {
    (void)dt;
    (void)screenH;
    if (g_id == MINIGAME_NONE) return MINIGAME_CONTINUE;
    if (IsKeyPressed(KEY_ESCAPE)) {
        Minigames_Clear();
        return MINIGAME_CANCELLED;
    }
    Vector2 mv = host_mouse_virtual();
    MinigameResult r = MINIGAME_CONTINUE;
    switch (g_id) {
    case MINIGAME_CARD_SWIPE: r = cardswipe_update(mv); break;
    case MINIGAME_MOP: r = mop_update(mv); break;
    case MINIGAME_DISHES: r = dishes_update(mv); break;
    case MINIGAME_RADIO: r = radio_update(mv); break;
    case MINIGAME_TRASH: r = trash_update(mv); break;
    case MINIGAME_FREEZER: r = freezer_update(mv); break;
    case MINIGAME_GENERATOR: r = gen_update(mv); break;
    default: break;
    }
    return r;
}

void Minigames_Draw(int screenW, int screenH) {
    if (g_id == MINIGAME_NONE) return;
    host_begin_texture();
    switch (g_id) {
    case MINIGAME_CARD_SWIPE:
        ClearBackground((Color){ 40, 40, 45, 255 });
        cardswipe_draw();
        break;
    case MINIGAME_MOP:
        ClearBackground((Color){ 240, 230, 210, 255 });
        mop_draw(host_mouse_virtual());
        break;
    case MINIGAME_DISHES:
        dishes_draw();
        break;
    case MINIGAME_RADIO:
        ClearBackground((Color){ 30, 30, 35, 255 });
        radio_draw();
        break;
    case MINIGAME_TRASH:
        ClearBackground((Color){ 50, 50, 60, 255 });
        trash_draw();
        break;
    case MINIGAME_FREEZER:
        ClearBackground((Color){ 30, 30, 35, 255 });
        freezer_draw();
        break;
    case MINIGAME_GENERATOR:
        gen_draw(host_mouse_virtual());
        break;
    default:
        break;
    }
    host_end_texture_draw(screenW, screenH);
    DrawRectangle(0, screenH - 28, screenW, 28, Fade(BLACK, 0.65f));
    DrawText("[ESC] Cancel minigame", 12, screenH - 22, 18, (Color){ 200, 195, 220, 255 });
}
