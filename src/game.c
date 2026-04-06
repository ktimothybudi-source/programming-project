/*
 * ============================================================================
 * game.c — STORY LOGIC, CAMERA, INTERACTIONS, DRAW ORDER
 * ============================================================================
 *
 * WHAT THIS FILE DOES (big picture):
 *   - Owns the struct Game: creates map, player, dialogue, audio, UI, anomalies.
 *   - Every frame: Game_Update reads input, moves player, checks collisions with
 *     interactables, advances days, starts dialogue.
 *   - Game_Draw draws world (with camera), then HUD, then dialogue on top.
 *
 * HOW TO READ THE CODE:
 *   - Search for "GameState" to see which mode we are in.
 *   - Game_HandleInteractions runs when player presses E; it uses a switch on
 *     InteractableType (defined in map.h).
 *   - Bool flags like clockedIn / moppingDone are the "memory" of what the player did.
 *
 * Sections marked "Code created by wu deguang": camera, spawn positions, on-screen
 * "Press E" prompts.
 * ============================================================================
 */

#include "game.h"
#include "player.h"
#include "map.h"
#include "dialogue.h"
#include "anomaly.h"
#include "audio.h"
#include "ui.h"
#include "intro_video.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void Game_ChangeState(Game *game, GameState newState);
static void Game_DevResetDialogue(Game *game);
static void Game_DevJumpToDay(Game *game, int day);
static void Game_DrawDevMenu(const Game *game);
static void Game_HandleInput(Game *game, float dt);
static void Game_HandleInteractions(Game *game);
static void Game_CompleteMinigame(Game *game);
static void Game_PlayMinigameStartSfx(Game *game, MinigameId id);
static void Game_UpdateObjective(Game *game);
static void Game_AdvanceDayIfReady(Game *game);
/* True when player should walk around the store (not title / not pure ending screen). */
static bool Game_IsPlayState(GameState s);
static void Game_LoadDay4KillerSprite(Game *game);
static void Game_UnloadDay4KillerSprite(Game *game);
static bool Game_ShouldDrawDay4Killer(const Game *game);
static void Game_DrawDay4Killer(const Game *game);
static void Game_TryTriggerDay4Boss(Game *game);

/* Code created by wu deguang — world (pixel) coordinates where the player appears. */
static const float SPAWN_KITCHEN_X = 550.0f * WORLD_SCALE;
static const float SPAWN_KITCHEN_Y = 1500.0f * WORLD_SCALE;
static const float SPAWN_CASHIER_X = 2400.0f * WORLD_SCALE;
static const float SPAWN_CASHIER_Y = 400.0f * WORLD_SCALE;
static const float SPAWN_FRONT_DOOR_X = 1400.0f * WORLD_SCALE;
static const float SPAWN_FRONT_DOOR_Y = 330.0f * WORLD_SCALE;
static const float SPAWN_BASEMENT_X = 1250.0f * WORLD_SCALE;
static const float SPAWN_BASEMENT_Y = 3000.0f * WORLD_SCALE;
static const float CAMERA_ZOOM = 0.48f;
static const float CAMERA_SMOOTH = 8.0f;

/*
 * Allocate Game, then allocate each subsystem. If anything failed we'd need more checks;
 * for this project we assume malloc succeeds except we check game pointer.
 */
Game *Game_Create(int screenWidth, int screenHeight) {
    Game *g = (Game *)malloc(sizeof(Game));
    if (!g) return NULL;

    g->screenWidth = screenWidth;
    g->screenHeight = screenHeight;

    g->map = Map_Create(screenWidth, screenHeight);
    g->player = Player_Create();
    g->dialogue = Dialogue_Create();
    g->audio = Audio_Create();
    g->ui = UI_Create();
    g->anomalies = Anomaly_Create(g);

    Minigames_Init();
    g->activeMinigame = MINIGAME_NONE;
    g->introVideo = IntroVideo_Open("assets/video/intro.mp4", screenWidth, screenHeight);

    g->paused = false;
    g->requestRestart = false;
    g->devMenuOpen = false;
    g->currentDay = 1;
    g->clockedIn = false;
    g->radioOn = false;
    g->dishesDone = false;
    g->moppingDone = false;
    g->garbageDone = false;
    g->freezerDone = false;
    g->generatorFixed = false;
    g->lightsOff = false;
    g->inBasement = false;
    g->hidingInLocker = false;
    g->ending1Triggered = false;
    g->ending2Triggered = false;
    g->ending4Triggered = false;
    g->day2RadioHeard = false;
    g->day2YoungLadyServed = false;
    g->day2OldManServed = false;
    g->day3BossCallHeard = false;
    g->day3FreezerDone = false;
    g->day3TeenBoyServed = false;
    g->day3OldLadyServed = false;
    g->day3CreepyManServed = false;
    g->day4ShamanLeft = false;
    g->day4FootstepsStarted = false;
    g->day4KillerVisible = false;
    g->day4BossDone = false;
    g->day4BlackoutAfterShaman = false;

    g->day4KillerSprite = (Texture2D){ 0 };
    g->day4KillerSpriteLoaded = false;

    g->objectiveText = "Press Enter to begin.";

    // Code created by wu deguang - Camera2D: center on screen, target kitchen spawn; zoom shows a wider view of the world
    g->camera.offset = (Vector2){ (float)g->screenWidth * 0.5f, (float)g->screenHeight * 0.5f };
    g->camera.target = (Vector2){ SPAWN_KITCHEN_X, SPAWN_KITCHEN_Y };
    g->camera.rotation = 0.0f;
    g->camera.zoom = CAMERA_ZOOM;

    Game_LoadDay4KillerSprite(g);

    Game_ChangeState(g, GAME_STATE_TITLE);
    return g;
}

/* Free in reverse order of creation; pointers become invalid after free. */
void Game_Destroy(Game *game) {
    if (!game) return;
    Game_UnloadDay4KillerSprite(game);
    IntroVideo_Close(game->introVideo);
    game->introVideo = NULL;
    Minigames_Shutdown();
    Anomaly_Destroy(game->anomalies);
    UI_Destroy(game->ui);
    Audio_Destroy(game->audio);
    Dialogue_Destroy(game->dialogue);
    Player_Destroy(game->player);
    Map_Destroy(game->map);
    free(game);
}

/*
 * Full reset: destroy all subsystems and recreate fresh, then return to title.
 * Used when player presses R (requestRestart).
 */
void Game_Restart(Game *game) {
    if (!game) return;
    Game_UnloadDay4KillerSprite(game);
    IntroVideo_Close(game->introVideo);
    game->introVideo = NULL;
    Map_Destroy(game->map);
    Player_Destroy(game->player);
    Dialogue_Destroy(game->dialogue);
    Audio_Destroy(game->audio);
    UI_Destroy(game->ui);
    Anomaly_Destroy(game->anomalies);

    game->map = Map_Create(game->screenWidth, game->screenHeight);
    game->player = Player_Create();
    game->dialogue = Dialogue_Create();
    game->audio = Audio_Create();
    game->ui = UI_Create();
    game->anomalies = Anomaly_Create(game);

    game->paused = false;
    game->requestRestart = false;
    game->devMenuOpen = false;
    game->currentDay = 1;
    game->clockedIn = false;
    game->radioOn = false;
    game->dishesDone = false;
    game->moppingDone = false;
    game->garbageDone = false;
    game->freezerDone = false;
    game->generatorFixed = false;
    game->lightsOff = false;
    game->inBasement = false;
    game->hidingInLocker = false;
    game->ending1Triggered = false;
    game->ending2Triggered = false;
    game->ending4Triggered = false;
    game->day2RadioHeard = false;
    game->day2YoungLadyServed = false;
    game->day2OldManServed = false;
    game->day3BossCallHeard = false;
    game->day3FreezerDone = false;
    game->day3TeenBoyServed = false;
    game->day3OldLadyServed = false;
    game->day3CreepyManServed = false;
    game->day4ShamanLeft = false;
    game->day4FootstepsStarted = false;
    game->day4KillerVisible = false;
    game->day4BossDone = false;
    game->day4BlackoutAfterShaman = false;
    game->objectiveText = "Press Enter to begin.";
    Minigames_Clear();
    game->activeMinigame = MINIGAME_NONE;
    Game_LoadDay4KillerSprite(game);
    Game_ChangeState(game, GAME_STATE_TITLE);
}

/* OB_STAIRS_UP: design x=1100 w=300 → center x=1250; optional nudge for sprite feet (see map.c). */
static const float DAY4_KILLER_X_OFFSET_DESIGN = 35.0f;

static float Day4_KillerFootYWorld(void) {
    return (2200.0f + 180.0f) * WORLD_SCALE;
}

static float Day4_KillerAnchorXWorld(void) {
    return ((1100.0f + 150.0f) + DAY4_KILLER_X_OFFSET_DESIGN) * WORLD_SCALE;
}

static float Day4_KillerSortYWorld(void) {
    const float dh = 220.0f * WORLD_SCALE;
    return Day4_KillerFootYWorld() - dh * 0.38f;
}

static void Game_LoadDay4KillerSprite(Game *game) {
    if (!game || game->day4KillerSpriteLoaded) return;
    game->day4KillerSprite = (Texture2D){ 0 };
    game->day4KillerSpriteLoaded = false;
    const char *path = "assets/sprites/characters/npc_mike_hawk.png";
    if (FileExists(path)) {
        game->day4KillerSprite = LoadTexture(path);
        game->day4KillerSpriteLoaded = (game->day4KillerSprite.id != 0);
        if (game->day4KillerSpriteLoaded)
            SetTextureFilter(game->day4KillerSprite, TEXTURE_FILTER_POINT);
    }
}

static void Game_UnloadDay4KillerSprite(Game *game) {
    if (!game || !game->day4KillerSpriteLoaded) return;
    if (game->day4KillerSprite.id != 0)
        UnloadTexture(game->day4KillerSprite);
    game->day4KillerSprite = (Texture2D){ 0 };
    game->day4KillerSpriteLoaded = false;
}

static bool Game_ShouldDrawDay4Killer(const Game *game) {
    if (!game) return false;
    if (game->currentDay != 4) return false;
    if (!game->generatorFixed || game->lightsOff) return false;
    if (!game->day4KillerVisible) return false;
    if (!game->inBasement) return false;
    if (game->day4FootstepsStarted) return false;
    return true;
}

/* Small knife drawn in world space (Mike faces left into the basement after texture flip). */
static void Game_DrawDay4KillerKnife(float stairCenterX, float footY, float dw, float dh) {
    const float s = WORLD_SCALE;
    /* Hand sits forward on his screen-right; blade angles up-left. */
    Vector2 hilt = { stairCenterX + dw * 0.11f, footY - dh * 0.46f };
    Vector2 tip = { hilt.x + 52.0f * s, hilt.y - 24.0f * s };
    float thick = 5.0f * s;
    DrawLineEx(hilt, tip, thick, (Color){ 235, 240, 248, 255 });
    DrawLineEx(hilt, tip, thick * 0.45f, (Color){ 180, 190, 205, 255 });
    Rectangle handle = { hilt.x - 10.0f * s, hilt.y - 5.0f * s, 22.0f * s, 11.0f * s };
    DrawRectanglePro(handle, (Vector2){ 4.0f * s, 5.5f * s }, -28.0f, (Color){ 48, 44, 52, 255 });
}

static void Game_DrawDay4Killer(const Game *game) {
    if (!Game_ShouldDrawDay4Killer(game)) return;

    const float stairCenterX = Day4_KillerAnchorXWorld();
    const float footY = Day4_KillerFootYWorld();

    if (game->day4KillerSpriteLoaded && game->day4KillerSprite.id != 0) {
        Texture2D tex = game->day4KillerSprite;
        float tw = (float)tex.width;
        float th = (float)tex.height;
        float dh = 220.0f * WORLD_SCALE;
        float dw = tw * (dh / th);
        /* Texture is left-heavy; anchor feet ~40% from sprite left so body sits on stair mid. */
        const float footAnchorX = 0.40f;
        float dx = stairCenterX - dw * footAnchorX;
        float dy = footY - dh;
        /* Face into basement (sprite assumed to face right — flip toward negative X). */
        Rectangle src = { (float)tex.width, 0.0f, -(float)tex.width, (float)tex.height };
        Rectangle dst = { dx, dy, dw, dh };
        DrawTexturePro(tex, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        Game_DrawDay4KillerKnife(stairCenterX, footY, dw, dh);
    } else {
        float r = 52.0f * WORLD_SCALE;
        Vector2 c = { stairCenterX, footY - r };
        DrawCircleV(c, r + 4.0f, (Color){ 20, 18, 28, 255 });
        DrawCircleV(c, r, (Color){ 180, 140, 110, 255 });
        Game_DrawDay4KillerKnife(stairCenterX, footY, r * 2.0f, r * 2.0f);
    }
}

static void Game_TryTriggerDay4Boss(Game *game) {
    if (!game || !game->player) return;
    if (!Game_ShouldDrawDay4Killer(game)) return;
    if (game->day4BossDone) return;
    if (Dialogue_IsActive(game->dialogue)) return;
    if (game->activeMinigame != MINIGAME_NONE) return;

    float kx = Day4_KillerAnchorXWorld();
    float ky = Day4_KillerFootYWorld();
    float dx = game->player->position.x - kx;
    float dy = game->player->position.y - ky;
    float r = 120.0f * WORLD_SCALE;
    if (dx * dx + dy * dy > r * r) return;

    game->activeMinigame = MINIGAME_BOSS;
    Minigames_Start(MINIGAME_BOSS);
    Game_PlayMinigameStartSfx(game, MINIGAME_BOSS);
}

static bool Game_IsPlayState(GameState s) {
    return s == GAME_STATE_DAY_1 || s == GAME_STATE_DAY_2 || s == GAME_STATE_DAY_3 || s == GAME_STATE_DAY_4
        || s == GAME_STATE_SCRIPTED_BLACKOUT || s == GAME_STATE_SCRIPTED_BASEMENT_CHASE;
}

/*
 * Switch mode: reset timer, optionally move player, start dialogue, set blood moon, etc.
 * This is how we go from INTRO -> DAY_1, or into blackout / ending.
 */
static void Game_ChangeState(Game *game, GameState newState) {
    game->state = newState;
    game->stateTime = 0.0f;

    if (newState == GAME_STATE_INTRO) {
        Dialogue_Start(game->dialogue, DIALOGUE_INTRO_DAY1, game);
        UI_StartFade(game->ui, 0.0f, 0.6f);
    } else if (newState == GAME_STATE_DAY_1) {
        game->player->position.x = SPAWN_FRONT_DOOR_X;
        game->player->position.y = SPAWN_FRONT_DOOR_Y;
    } else if (newState == GAME_STATE_DAY_2) {
        game->player->position.x = SPAWN_FRONT_DOOR_X;
        game->player->position.y = SPAWN_FRONT_DOOR_Y;
        game->clockedIn = false;
        game->radioOn = false;
        game->dishesDone = false;
        game->moppingDone = false;
        game->garbageDone = false;
    } else if (newState == GAME_STATE_DAY_3) {
        game->player->position.x = SPAWN_FRONT_DOOR_X;
        game->player->position.y = SPAWN_FRONT_DOOR_Y;
        game->clockedIn = false;
        game->radioOn = false;
        game->dishesDone = false;
        game->moppingDone = false;
        game->garbageDone = false;
    } else if (newState == GAME_STATE_DAY_4) {
        game->map->bloodMoon = true;
        game->player->position.x = SPAWN_FRONT_DOOR_X;
        game->player->position.y = SPAWN_FRONT_DOOR_Y;
        game->clockedIn = false;
    } else if (newState == GAME_STATE_SCRIPTED_BLACKOUT) {
        game->lightsOff = true;
        Dialogue_Start(game->dialogue, DIALOGUE_DAY4_LIGHTS_OUT, game);
    } else if (newState == GAME_STATE_ENDING_1) {
        game->ending1Triggered = true;
        Dialogue_Start(game->dialogue, DIALOGUE_ENDING_1, game);
    } else if (newState == GAME_STATE_ENDING_2) {
        game->ending2Triggered = true;
        Dialogue_Start(game->dialogue, DIALOGUE_ENDING_2, game);
        Audio_PlaySfx(game->audio, AUDIO_SFX_KNIFE);
    } else if (newState == GAME_STATE_ENDING_4) {
        game->ending4Triggered = true;
        Dialogue_Start(game->dialogue, DIALOGUE_ENDING_4, game);
    }
}

static void Game_DevResetDialogue(Game *game) {
    if (!game || !game->dialogue) return;
    DialogueSystem *d = game->dialogue;
    d->active = false;
    d->current = NULL;
    d->currentId = DIALOGUE_NONE;
    d->lastClosedId = DIALOGUE_NONE;
    d->justClosed = false;
    d->currentIndex = 0;
    d->visibleChars = 0;
    d->typewriterAcc = 0.0f;
}

static void Game_DevJumpToDay(Game *game, int day) {
    if (!game || day < 1 || day > 4) return;
    game->paused = false;
    game->devMenuOpen = false;
    Minigames_Clear();
    game->activeMinigame = MINIGAME_NONE;
    Game_DevResetDialogue(game);

    game->freezerDone = false;
    game->generatorFixed = false;
    game->lightsOff = false;
    game->inBasement = false;
    game->hidingInLocker = false;
    game->ending1Triggered = false;
    game->ending2Triggered = false;
    game->ending4Triggered = false;
    game->day4ShamanLeft = false;
    game->day4FootstepsStarted = false;
    game->day4KillerVisible = false;
    game->day4BossDone = false;
    game->day4BlackoutAfterShaman = false;

    game->day2RadioHeard = false;
    game->day2YoungLadyServed = false;
    game->day2OldManServed = false;
    game->day3BossCallHeard = false;
    game->day3FreezerDone = false;
    game->day3TeenBoyServed = false;
    game->day3OldLadyServed = false;
    game->day3CreepyManServed = false;

    if (day == 1) {
        game->map->bloodMoon = false;
        game->currentDay = 1;
        game->clockedIn = false;
        game->radioOn = false;
        game->dishesDone = false;
        game->moppingDone = false;
        game->garbageDone = false;
        Game_ChangeState(game, GAME_STATE_DAY_1);
        game->camera.target = (Vector2){ game->player->position.x, game->player->position.y };
        return;
    }
    if (day == 2) {
        game->map->bloodMoon = false;
        game->currentDay = 2;
        Game_ChangeState(game, GAME_STATE_DAY_2);
        game->camera.target = (Vector2){ game->player->position.x, game->player->position.y };
        return;
    }
    if (day == 3) {
        game->map->bloodMoon = false;
        game->currentDay = 3;
        game->day2RadioHeard = true;
        game->day2YoungLadyServed = true;
        game->day2OldManServed = true;
        Game_ChangeState(game, GAME_STATE_DAY_3);
        game->camera.target = (Vector2){ game->player->position.x, game->player->position.y };
        return;
    }
    game->currentDay = 4;
    game->day2RadioHeard = true;
    game->day2YoungLadyServed = true;
    game->day2OldManServed = true;
    game->day3BossCallHeard = true;
    game->day3FreezerDone = true;
    game->day3TeenBoyServed = true;
    game->day3OldLadyServed = true;
    game->day3CreepyManServed = true;
    Game_ChangeState(game, GAME_STATE_DAY_4);
    game->camera.target = (Vector2){ game->player->position.x, game->player->position.y };
}

static void Game_DrawDevMenu(const Game *game) {
    if (!game || !game->devMenuOpen) return;
    int sw = game->screenWidth;
    int sh = game->screenHeight;
    int boxW = 420;
    int boxH = 220;
    int bx = sw / 2 - boxW / 2;
    int by = sh / 2 - boxH / 2;
    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 140 });
    DrawRectangle(bx, by, boxW, boxH, (Color){ 18, 14, 28, 250 });
    DrawRectangleLinesEx((Rectangle){ (float)bx, (float)by, (float)boxW, (float)boxH }, 2,
                           (Color){ 160, 140, 220, 255 });
    int y = by + 18;
    int fs = 20;
    const char *t = "DEVELOPER — Skip to day";
    DrawText(t, bx + boxW / 2 - MeasureText(t, fs) / 2, y, fs, (Color){ 240, 230, 255, 255 });
    y += 36;
    fs = 16;
    DrawText("[1] Day 1   [2] Day 2   [3] Day 3   [4] Day 4", bx + 24, y, fs, (Color){ 210, 200, 240, 255 });
    y += 28;
    DrawText("Day 4 jump: prior days marked complete.", bx + 24, y, 14, (Color){ 160, 150, 190, 255 });
    y += 22;
    DrawText("[F] or [Esc] close", bx + 24, y, 14, (Color){ 140, 130, 170, 255 });
}

/*
 * Title: Enter starts intro. Esc toggles pause. R requests restart.
 * Ending: Enter advances through final dialogue back to title.
 */
static void Game_HandleInput(Game *game, float dt) {
    (void)dt;

    if (game->introVideo)
        return;

    if (IsKeyPressed(KEY_R)) {
        game->requestRestart = true;
        return;
    }

    if (IsKeyPressed(KEY_F))
        game->devMenuOpen = !game->devMenuOpen;

    if (game->devMenuOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->devMenuOpen = false;
            return;
        }
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
            Game_DevJumpToDay(game, 1);
            return;
        }
        if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
            Game_DevJumpToDay(game, 2);
            return;
        }
        if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
            Game_DevJumpToDay(game, 3);
            return;
        }
        if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) {
            Game_DevJumpToDay(game, 4);
            return;
        }
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE) && game->state != GAME_STATE_TITLE) {
        game->paused = !game->paused;
    }

    if (game->state == GAME_STATE_TITLE) {
        if (IsKeyPressed(KEY_ENTER)) {
            Game_ChangeState(game, GAME_STATE_INTRO);
        }
        return;
    }

    if (game->state == GAME_STATE_ENDING_1 || game->state == GAME_STATE_ENDING_2
        || game->state == GAME_STATE_ENDING_4) {
        if (IsKeyPressed(KEY_ENTER) && !Dialogue_IsActive(game->dialogue)) {
            Dialogue_Start(game->dialogue, DIALOGUE_THE_END, game);
        }
        if (Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_THE_END) {
            Game_ChangeState(game, GAME_STATE_TITLE);
        }
        if (Dialogue_JustClosed(game->dialogue)) {
            DialogueId lid = game->dialogue->lastClosedId;
            if (lid == DIALOGUE_ENDING_1 || lid == DIALOGUE_ENDING_2 || lid == DIALOGUE_ENDING_4)
                Dialogue_Start(game->dialogue, DIALOGUE_THE_END, game);
        }
        return;
    }
}

/* When end-of-day conditions are met, bump currentDay and play closing dialogue. */
static void Game_AdvanceDayIfReady(Game *game) {
    if (game->currentDay == 1 && game->clockedIn && game->moppingDone && game->dishesDone && game->garbageDone) {
        if (!Dialogue_IsActive(game->dialogue)) {
            Dialogue_Start(game->dialogue, DIALOGUE_DAY1_END_OF_DAY, game);
            game->currentDay = 2;
        }
    }
    if (game->currentDay == 2 && game->day2OldManServed && !Dialogue_IsActive(game->dialogue)) {
        Dialogue_Start(game->dialogue, DIALOGUE_DAY2_END, game);
        game->currentDay = 3;
    }
    if (game->currentDay == 3 && game->day3CreepyManServed && !Dialogue_IsActive(game->dialogue)) {
        Dialogue_Start(game->dialogue, DIALOGUE_DAY3_END, game);
        game->currentDay = 4;
    }
}

/* Returns true if the given interactable type is the current objective task (enforce one task at a time). */
static bool Game_IsCurrentObjectiveTask(const Game *game, InteractableType type) {
    if (!game) return false;
    
    if (game->currentDay == 1) {
        if (!game->clockedIn) return type == INTERACT_BADGE;
        if (!game->moppingDone) return type == INTERACT_MOP;
        if (!game->dishesDone) return type == INTERACT_SINK;
        if (!game->garbageDone) return type == INTERACT_GARBAGE;
        if (game->clockedIn && game->moppingDone && game->dishesDone && game->garbageDone) return type == INTERACT_CLOCK_OUT;
        return false;
    }
    
    if (game->currentDay == 2) {
        if (!game->clockedIn) return type == INTERACT_BADGE;
        if (!game->day2RadioHeard) return type == INTERACT_RADIO;
        return false;  // Customer serving is handled by cashier, not here
    }
    
    if (game->currentDay == 3) {
        if (!game->clockedIn) return type == INTERACT_BADGE;
        if (!game->day3FreezerDone) return type == INTERACT_FREEZER_DOOR;
        return false;  // Customer serving is handled by cashier, not here
    }
    
    if (game->currentDay == 4) {
        if (!game->clockedIn) return type == INTERACT_BADGE;
        if (game->lightsOff && !game->inBasement) return type == INTERACT_BASEMENT_STAIRS;
        if (game->lightsOff && game->inBasement && !game->generatorFixed) return type == INTERACT_GENERATOR;
        if (game->generatorFixed && game->inBasement && !game->day4FootstepsStarted && game->day4BossDone)
            return type == INTERACT_BASEMENT_STAIRS_UP;
        if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && !game->hidingInLocker) 
            return type == INTERACT_LOCKER_1 || type == INTERACT_LOCKER_2 || type == INTERACT_LOCKER_3;
        return false;
    }
    
    return false;
}

/* Cashier / shaman dialogue only after that day's prior tasks (clock in, radio, freezer, etc.). */
static bool Game_CashierCustomerPrereqsMet(const Game *game) {
    if (!game) return false;
    if (game->currentDay == 2)
        return game->clockedIn && game->day2RadioHeard;
    if (game->currentDay == 3)
        return game->clockedIn && game->day3BossCallHeard && game->day3FreezerDone;
    if (game->currentDay == 4)
        return game->clockedIn;
    return false;
}

static void Game_PlayMinigameStartSfx(Game *game, MinigameId id) {
    if (!game || !game->audio) return;
    switch (id) {
    case MINIGAME_CARD_SWIPE:
        Audio_PlaySfx(game->audio, AUDIO_SFX_SWIPE);
        break;
    case MINIGAME_MOP:
        Audio_PlaySfx(game->audio, AUDIO_SFX_MOP_DRAG);
        break;
    case MINIGAME_DISHES:
        Audio_PlaySfx(game->audio, AUDIO_SFX_DISHES);
        break;
    case MINIGAME_RADIO:
        Audio_PlaySfx(game->audio, AUDIO_SFX_RADIO_ON);
        break;
    case MINIGAME_BOSS:
        Audio_PlaySfx(game->audio, AUDIO_SFX_KNIFE);
        break;
    default:
        break;
    }
}

static void Game_CompleteMinigame(Game *game) {
    if (game->audio)
        Audio_StopAllSfx(game->audio);
    MinigameId m = game->activeMinigame;
    game->activeMinigame = MINIGAME_NONE;
    Minigames_Clear();
    switch (m) {
    case MINIGAME_CARD_SWIPE:
        game->clockedIn = true;
        if (game->currentDay == 1)
            Dialogue_Start(game->dialogue, DIALOGUE_DAY1_CLOCKED_IN, game);
        break;
    case MINIGAME_MOP:
        game->moppingDone = true;
        break;
    case MINIGAME_DISHES:
        game->dishesDone = true;
        break;
    case MINIGAME_RADIO:
        game->radioOn = true;
        Audio_PlaySfx(game->audio, AUDIO_SFX_RADIO_TUNE);
        if (game->currentDay == 2 && !game->day2RadioHeard) {
            game->day2RadioHeard = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY2_RADIO_NEWS, game);
        }
        break;
    case MINIGAME_TRASH:
        game->garbageDone = true;
        Audio_PlaySfx(game->audio, AUDIO_SFX_TRASH);
        break;
    case MINIGAME_FREEZER:
        if (game->currentDay == 3) {
            game->day3FreezerDone = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY3_FREEZER_DONE, game);
        }
        break;
    case MINIGAME_GENERATOR:
        game->generatorFixed = true;
        game->lightsOff = false;
        game->day4KillerVisible = true;
        Dialogue_Start(game->dialogue, DIALOGUE_DAY4_GENERATOR_FIXED, game);
        break;
    case MINIGAME_BOSS:
        Game_Restart(game);
        return;
    default:
        break;
    }
}

/*
 * If player pressed E: find overlapping interactable trigger, run the matching case.
 * If none, but player is in cashier rectangle, serve the next customer by day.
 * Early "return" after handling one thing so we do not trigger two actions at once.
 */
static void Game_HandleInteractions(Game *game) {
    if (!game || !game->player || !game->map) return;
    if (game->activeMinigame != MINIGAME_NONE) return;
    if (Dialogue_IsActive(game->dialogue)) return;

    if (!IsKeyPressed(KEY_E)) return;

    Rectangle playerRect = Player_GetBounds(game->player);
    int n = 0;
    const Interactable *interactables = Map_GetInteractables(game->map, &n);

    for (int i = 0; i < n; i++) {
        const Interactable *in = &interactables[i];
        Rectangle useZone = (in->triggerZone.width > 0 && in->triggerZone.height > 0) ? in->triggerZone : in->bounds;
        if (!CheckCollisionRecs(playerRect, useZone)) continue;

        // Only allow interaction with current objective task
        if (!Game_IsCurrentObjectiveTask(game, in->type)) continue;

        switch (in->type) {
            case INTERACT_BADGE:
                if (!game->clockedIn) {
                    game->activeMinigame = MINIGAME_CARD_SWIPE;
                    Minigames_Start(MINIGAME_CARD_SWIPE);
                    Game_PlayMinigameStartSfx(game, MINIGAME_CARD_SWIPE);
                }
                return;
            case INTERACT_CLOCK_OUT:
                if (game->currentDay == 1 && game->clockedIn && game->moppingDone && game->dishesDone && game->garbageDone) {
                    Game_AdvanceDayIfReady(game);
                }
                return;
            case INTERACT_SINK:
                if (!game->dishesDone) {
                    game->activeMinigame = MINIGAME_DISHES;
                    Minigames_Start(MINIGAME_DISHES);
                    Game_PlayMinigameStartSfx(game, MINIGAME_DISHES);
                }
                return;
            case INTERACT_MOP:
                if (!game->moppingDone) {
                    game->activeMinigame = MINIGAME_MOP;
                    Minigames_Start(MINIGAME_MOP);
                    Game_PlayMinigameStartSfx(game, MINIGAME_MOP);
                }
                return;
            case INTERACT_RADIO:
                if (!game->radioOn) {
                    game->activeMinigame = MINIGAME_RADIO;
                    Minigames_Start(MINIGAME_RADIO);
                    Game_PlayMinigameStartSfx(game, MINIGAME_RADIO);
                }
                return;
            case INTERACT_GARBAGE:
                if (!game->garbageDone) {
                    game->activeMinigame = MINIGAME_TRASH;
                    Minigames_Start(MINIGAME_TRASH);
                }
                return;
            case INTERACT_FREEZER_DOOR:
                if (game->currentDay == 3 && !game->day3FreezerDone) {
                    game->activeMinigame = MINIGAME_FREEZER;
                    Minigames_Start(MINIGAME_FREEZER);
                }
                return;
            case INTERACT_BASEMENT_STAIRS:
                if (game->state == GAME_STATE_DAY_4 && game->lightsOff && !game->inBasement) {
                    game->player->position.x = SPAWN_BASEMENT_X;
                    game->player->position.y = SPAWN_BASEMENT_Y;
                    game->inBasement = true;
                }
                return;
            case INTERACT_BASEMENT_STAIRS_UP:
                if (game->state == GAME_STATE_DAY_4 && game->generatorFixed && game->inBasement && !game->day4FootstepsStarted) {
                    game->player->position.x = SPAWN_CASHIER_X;
                    game->player->position.y = 900.0f * WORLD_SCALE;
                    game->inBasement = false;
                    game->day4FootstepsStarted = true;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY4_FOOTSTEPS, game);
                    Audio_PlaySfx(game->audio, AUDIO_SFX_FOOTSTEPS);
                }
                return;
            case INTERACT_GENERATOR:
                if (game->inBasement && !game->generatorFixed) {
                    game->activeMinigame = MINIGAME_GENERATOR;
                    Minigames_Start(MINIGAME_GENERATOR);
                }
                return;
            case INTERACT_LOCKER_1:
            case INTERACT_LOCKER_2:
            case INTERACT_LOCKER_3:
                if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && !game->hidingInLocker) {
                    game->hidingInLocker = true;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY4_HIDE_IN_LOCKER, game);
                    Audio_PlaySfx(game->audio, AUDIO_SFX_LOCKER);
                }
                return;
            default:
                break;
        }
    }

    // NPCs at cashier (check by day and proximity to cashier area)
    int npcCount = 0;
    const NpcSpot *npcs = Map_GetNpcs(game->map, &npcCount);
    Rectangle cashierArea = game->map->cashierBounds;
    if (!CheckCollisionRecs(playerRect, cashierArea)) return;

    if ((game->currentDay == 2 || game->currentDay == 3 || game->currentDay == 4)
        && !Game_CashierCustomerPrereqsMet(game))
        return;

    if (game->currentDay == 2) {
        if (!game->day2YoungLadyServed) {
            game->day2YoungLadyServed = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY2_YOUNG_LADY, game);
            return;
        }
        if (!game->day2OldManServed && game->day2YoungLadyServed) {
            game->day2OldManServed = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY2_OLD_MAN, game);
            return;
        }
    }
    if (game->currentDay == 3) {
        if (!game->day3TeenBoyServed) {
            game->day3TeenBoyServed = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY3_TEEN_BOY, game);
            return;
        }
        if (!game->day3OldLadyServed && game->day3TeenBoyServed) {
            game->day3OldLadyServed = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY3_OLD_LADY, game);
            return;
        }
        if (!game->day3CreepyManServed && game->day3OldLadyServed) {
            game->day3CreepyManServed = true;
            Dialogue_Start(game->dialogue, DIALOGUE_DAY3_CREEPY_MAN, game);
            return;
        }
    }
    if (game->currentDay == 4 && !game->day4ShamanLeft) {
        game->day4ShamanLeft = true;
        game->day4BlackoutAfterShaman = true;
        Dialogue_Start(game->dialogue, DIALOGUE_DAY4_SHAMAN, game);
        return;
    }
    (void)npcs;
}

// Code created by wu deguang - returns the "Press E to ..." string for an interactable based on game state
static const char *Game_GetInteractPrompt(const Game *game, const Interactable *in) {
    if (!game || !in) return NULL;

    if (in->type == INTERACT_BASEMENT_STAIRS_UP && game->currentDay == 4 && game->state == GAME_STATE_DAY_4
        && game->generatorFixed && game->inBasement && !game->day4FootstepsStarted && !game->day4BossDone)
        return "Confront Mike Hawk on the stairs first";

    // Only show prompt if this is the current objective task
    if (!Game_IsCurrentObjectiveTask(game, in->type)) return NULL;
    
    switch (in->type) {
        case INTERACT_BADGE: return game->clockedIn ? NULL : "Press E to Clock In";
        case INTERACT_CLOCK_OUT: return (game->currentDay == 1 && game->clockedIn && game->moppingDone && game->dishesDone && game->garbageDone) ? "Press E to Clock Out" : NULL;
        case INTERACT_SINK: return !game->dishesDone ? "Press E to Wash Dishes" : NULL;
        case INTERACT_MOP: return !game->moppingDone ? "Press E to Mop the Floor" : NULL;
        case INTERACT_RADIO: return !game->radioOn ? "Press E to Turn On Radio" : NULL;
        case INTERACT_GARBAGE: return !game->garbageDone ? "Press E to Take Out Garbage" : NULL;
        case INTERACT_FREEZER_DOOR: return (game->currentDay == 3 && !game->day3FreezerDone) ? "Press E to Remove Expired Meat" : NULL;
        case INTERACT_BASEMENT_STAIRS: return (game->state == GAME_STATE_DAY_4 && game->lightsOff && !game->inBasement) ? "Press E to Go Downstairs" : NULL;
        case INTERACT_BASEMENT_STAIRS_UP: return (game->state == GAME_STATE_DAY_4 && game->generatorFixed && game->inBasement && !game->day4FootstepsStarted) ? "Press E to Go Upstairs" : NULL;
        case INTERACT_GENERATOR: return (game->lightsOff && game->inBasement && !game->generatorFixed) ? "Press E to Fix Generator" : NULL;
        case INTERACT_LOCKER_1:
        case INTERACT_LOCKER_2:
        case INTERACT_LOCKER_3: return (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && !game->hidingInLocker) ? "Press E to Hide" : NULL;
        default: return NULL;
    }
}

// Code created by wu deguang - "Press E to Serve Customer" / "Meet the Shaman" when at cashier
static const char *Game_GetCashierPrompt(const Game *game) {
    if (!game) return NULL;
    if (!Game_CashierCustomerPrereqsMet(game)) return NULL;
    if (game->currentDay == 2) {
        if (!game->day2YoungLadyServed) return "Press E to Serve Customer";
        if (!game->day2OldManServed) return "Press E to Serve Customer";
    }
    if (game->currentDay == 3) {
        if (!game->day3TeenBoyServed) return "Press E to Serve Customer";
        if (!game->day3OldLadyServed) return "Press E to Serve Customer";
        if (!game->day3CreepyManServed) return "Press E to Serve Customer";
    }
    if (game->currentDay == 4 && !game->day4ShamanLeft) return "Press E to Meet the Shaman";
    return NULL;
}

/* Sets objectiveText pointer to a string literal — shown in HUD top-left. */
static void Game_UpdateObjective(Game *game) {
    if (game->introVideo) {
        game->objectiveText = "Opening video — Enter / Space / Click to skip.";
        return;
    }
    if (game->state == GAME_STATE_TITLE) {
        game->objectiveText = "Press Enter to begin.";
        return;
    }
    if (game->state == GAME_STATE_INTRO) {
        game->objectiveText = "Listen to the intro.";
        return;
    }
    if (game->state == GAME_STATE_ENDING_1 || game->state == GAME_STATE_ENDING_2
        || game->state == GAME_STATE_ENDING_4) {
        game->objectiveText = "Read the ending. Press Enter, then advance to title.";
        return;
    }
    if (game->state == GAME_STATE_DAY4_STAIRS_CHOICE) {
        game->objectiveText = "Choose: [1] Hide (Ending 1)  [2] Weapon (Ending 2)  [3] Shout (Ending 4)";
        return;
    }

    if (game->currentDay == 1) {
        if (!game->clockedIn) game->objectiveText = "Day 1: Clock in at the badge reader.";
        else if (!game->moppingDone) game->objectiveText = "Day 1: Mop the floor.";
        else if (!game->dishesDone) game->objectiveText = "Day 1: Wash the dishes.";
        else if (!game->garbageDone) game->objectiveText = "Day 1: Take out the garbage.";
        else game->objectiveText = "Day 1: Clock out to end the day.";
        return;
    }
    if (game->currentDay == 2) {
        if (!game->clockedIn) game->objectiveText = "Day 2: Clock in.";
        else if (!game->day2RadioHeard) game->objectiveText = "Day 2: Turn on the radio.";
        else if (!game->day2YoungLadyServed) game->objectiveText = "Day 2: Serve the customer at the cashier.";
        else if (!game->day2OldManServed) game->objectiveText = "Day 2: Serve the old man.";
        else game->objectiveText = "Day 2: Clock out.";
        return;
    }
    if (game->currentDay == 3) {
        if (!game->clockedIn) game->objectiveText = "Day 3: Clock in. Answer the phone (boss will call).";
        else if (!game->day3BossCallHeard) game->objectiveText = "Day 3: Wait for boss call or go to cashier.";
        else if (!game->day3FreezerDone) game->objectiveText = "Day 3: Remove expired meat from freezer.";
        else if (!game->day3TeenBoyServed) game->objectiveText = "Day 3: Serve customers at cashier.";
        else if (!game->day3OldLadyServed) game->objectiveText = "Day 3: Serve the old lady.";
        else if (!game->day3CreepyManServed) game->objectiveText = "Day 3: Serve the last customer.";
        else game->objectiveText = "Day 3: Clock out.";
        return;
    }
    if (game->currentDay == 4) {
        if (!game->clockedIn) game->objectiveText = "Day 4: Clock in.";
        else if (!game->day4ShamanLeft) game->objectiveText = "Day 4: Go to cashier (shaman will appear).";
        else if (!game->lightsOff) {
            if (Dialogue_IsActive(game->dialogue) && game->dialogue->currentId == DIALOGUE_DAY4_SHAMAN)
                game->objectiveText = "Day 4: Listen to the shaman.";
            else
                game->objectiveText = "Day 4: Blackout is coming — then use the basement stairs.";
        }
        else if (!game->generatorFixed) game->objectiveText = "Day 4: Go to basement and fix the generator.";
        else if (!game->day4BossDone)
            game->objectiveText = "Day 4: Get close to Mike Hawk on the stairs — needle minigame: click when it hits green.";
        else if (!game->day4FootstepsStarted) game->objectiveText = "Day 4: Go back up the stairs.";
        else if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && !game->hidingInLocker)
            game->objectiveText = "Day 4: Hide in a locker (Ending 1) — or you already chose another path.";
        else game->objectiveText = "Day 4: Survive the night.";
        return;
    }
}

/*
 * Central per-frame update: input, optional restart, UI/dialogue/audio tick,
 * then branch on state (intro vs play vs ending). Play state moves player,
 * updates camera, checks story timers, interactions, day advancement.
 */
void Game_Update(Game *game, float dt) {
    if (!game) return;

    game->stateTime += dt;

    if (game->introVideo) {
        if (IsKeyPressed(KEY_R))
            game->requestRestart = true;
        if (game->requestRestart) {
            Game_Restart(game);
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)
            || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            IntroVideo_Close(game->introVideo);
            game->introVideo = NULL;
        } else if (!IntroVideo_Update(game->introVideo, dt)) {
            IntroVideo_Close(game->introVideo);
            game->introVideo = NULL;
        }
        UI_Update(game->ui, dt);
        Audio_Update(game->audio);
        Game_UpdateObjective(game);
        return;
    }

    if (game->activeMinigame != MINIGAME_NONE) {
        if (IsKeyPressed(KEY_R))
            game->requestRestart = true;
        if (game->requestRestart) {
            Game_Restart(game);
            return;
        }
        MinigameId mid = game->activeMinigame;
        MinigameResult mr = Minigames_Update(dt, game->screenWidth, game->screenHeight);
        if (mr == MINIGAME_WON)
            Game_CompleteMinigame(game);
        else if (mr == MINIGAME_LOST && mid == MINIGAME_BOSS) {
            if (game->audio)
                Audio_StopAllSfx(game->audio);
            game->activeMinigame = MINIGAME_NONE;
            Minigames_Clear();
            Game_ChangeState(game, GAME_STATE_ENDING_1);
        } else if (mr == MINIGAME_CANCELLED) {
            if (game->audio)
                Audio_StopAllSfx(game->audio);
            game->activeMinigame = MINIGAME_NONE;
            Minigames_Clear();
        }
        UI_Update(game->ui, dt);
        Dialogue_Update(game->dialogue);
        Audio_Update(game->audio);
        Game_UpdateObjective(game);
        return;
    }

    Game_HandleInput(game, dt);

    if (game->requestRestart) {
        Game_Restart(game);
        return;
    }

    bool allowWorldUpdate = !game->paused && !game->devMenuOpen && game->activeMinigame == MINIGAME_NONE;
    UI_Update(game->ui, dt);
    if (!game->devMenuOpen && game->activeMinigame == MINIGAME_NONE)
        Dialogue_Update(game->dialogue);
    Audio_Update(game->audio);

    if (game->state == GAME_STATE_INTRO) {
        if (!Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue)) {
            Game_ChangeState(game, GAME_STATE_DAY_1);
        }
        Game_UpdateObjective(game);
        return;
    }

    if (game->state == GAME_STATE_DAY4_STAIRS_CHOICE) {
        if (allowWorldUpdate && !Dialogue_IsActive(game->dialogue) && game->activeMinigame == MINIGAME_NONE) {
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1))
                game->state = GAME_STATE_SCRIPTED_BASEMENT_CHASE;
            else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))
                Game_ChangeState(game, GAME_STATE_ENDING_2);
            else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))
                Game_ChangeState(game, GAME_STATE_ENDING_4);
        }
        Anomaly_Update(game->anomalies, game, dt);
        Game_UpdateObjective(game);
        return;
    }

    if (Game_IsPlayState(game->state)) {
        /* Old runs only had day4ShamanLeft; arm blackout if we're still before lights-out. */
        if (game->currentDay == 4 && game->day4ShamanLeft && !game->generatorFixed
            && !game->lightsOff && !game->day4BlackoutAfterShaman)
            game->day4BlackoutAfterShaman = true;

        /*
         * Day N -> correct GameState when the calendar advanced but state lagged (e.g. player
         * closed end-of-day dialogue while paused). Must not sit behind allowWorldUpdate.
         */
        if (game->currentDay == 2 && game->state == GAME_STATE_DAY_1 && !Dialogue_IsActive(game->dialogue)) {
            Game_ChangeState(game, GAME_STATE_DAY_2);
        }
        if (game->currentDay == 3 && game->state == GAME_STATE_DAY_2 && !Dialogue_IsActive(game->dialogue)) {
            Game_ChangeState(game, GAME_STATE_DAY_3);
        }
        if (game->currentDay == 4 && game->state == GAME_STATE_DAY_3 && !Dialogue_IsActive(game->dialogue)) {
            Game_ChangeState(game, GAME_STATE_DAY_4);
        }

        /*
         * Shaman -> blackout: day4BlackoutAfterShaman is set when the shaman script successfully
         * starts (cashier). When dialogue goes idle, we always enter blackout. Also accept
         * lastClosedId for saves stuck from older builds.
         * After the generator is fixed, lights stay bright — do not fire blackout again.
         */
        if (game->currentDay == 4 && game->day4ShamanLeft && !game->generatorFixed && !game->lightsOff
            && !Dialogue_IsActive(game->dialogue)
            && game->state != GAME_STATE_SCRIPTED_BLACKOUT && game->state != GAME_STATE_DAY4_STAIRS_CHOICE
            && (game->day4BlackoutAfterShaman || game->dialogue->lastClosedId == DIALOGUE_DAY4_SHAMAN)) {
            game->day4BlackoutAfterShaman = false;
            Game_ChangeState(game, GAME_STATE_SCRIPTED_BLACKOUT);
        }

        if (game->state == GAME_STATE_SCRIPTED_BLACKOUT && !Dialogue_IsActive(game->dialogue)
            && game->dialogue->lastClosedId == DIALOGUE_DAY4_LIGHTS_OUT) {
            game->state = GAME_STATE_DAY_4;
        }

        if (game->day4FootstepsStarted && game->state == GAME_STATE_DAY_4 && !Dialogue_IsActive(game->dialogue)
            && game->dialogue->lastClosedId == DIALOGUE_DAY4_FOOTSTEPS) {
            game->state = GAME_STATE_DAY4_STAIRS_CHOICE;
        }

        if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && game->hidingInLocker && !Dialogue_IsActive(game->dialogue)
            && game->dialogue->lastClosedId == DIALOGUE_DAY4_HIDE_IN_LOCKER) {
            Game_ChangeState(game, GAME_STATE_ENDING_1);
        }

        if (allowWorldUpdate) {
            Player_Update(game->player, game->map, dt, !Dialogue_IsActive(game->dialogue));
            game->inBasement = (Map_GetRegionAt(game->map, game->player->position.x, game->player->position.y) == REGION_BASEMENT);

            // Code created by wu deguang - smooth camera follow, clamped to world bounds
            float hw = (game->screenWidth * 0.5f) / game->camera.zoom;
            float hh = (game->screenHeight * 0.5f) / game->camera.zoom;
            game->camera.target.x += (game->player->position.x - game->camera.target.x) * CAMERA_SMOOTH * dt;
            game->camera.target.y += (game->player->position.y - game->camera.target.y) * CAMERA_SMOOTH * dt;
            if (game->camera.target.x < hw) game->camera.target.x = hw;
            if (game->camera.target.x > WORLD_WIDTH - hw) game->camera.target.x = WORLD_WIDTH - hw;
            if (game->camera.target.y < hh) game->camera.target.y = hh;
            if (game->camera.target.y > WORLD_HEIGHT - hh) game->camera.target.y = WORLD_HEIGHT - hh;

            if (game->currentDay == 3 && !game->day3BossCallHeard && game->stateTime > 2.0f) {
                game->day3BossCallHeard = true;
                Dialogue_Start(game->dialogue, DIALOGUE_DAY3_BOSS_CALL, game);
            }

            Game_TryTriggerDay4Boss(game);
            Game_HandleInteractions(game);
            Game_AdvanceDayIfReady(game);
        }
        Anomaly_Update(game->anomalies, game, dt);
    }

    Game_UpdateObjective(game);
}

/*
 * Draw order matters: title is just UI. In gameplay, BeginMode2D applies the camera
 * transform so map coordinates match world space. After EndMode2D, coordinates are
 * screen pixels again (HUD, dialogue, "Press E" bar).
 */
void Game_Draw(Game *game) {
    if (!game) return;

    if (game->introVideo) {
        int sw = game->screenWidth;
        int sh = game->screenHeight;
        ClearBackground(BLACK);
        Texture2D t = IntroVideo_GetTexture(game->introVideo);
        if (t.id != 0) {
            float tw = (float)t.width;
            float th = (float)t.height;
            float scale = fminf((float)sw / tw, (float)sh / th);
            float dw = tw * scale;
            float dh = th * scale;
            float ox = ((float)sw - dw) * 0.5f;
            float oy = ((float)sh - dh) * 0.5f;
            DrawTexturePro(
                t,
                (Rectangle){ 0.0f, 0.0f, tw, th },
                (Rectangle){ ox, oy, dw, dh },
                (Vector2){ 0.0f, 0.0f },
                0.0f,
                WHITE);
        }
        const char *hint = "Enter / Space / Click — skip";
        int fs = 18;
        DrawText(hint, sw / 2 - MeasureText(hint, fs) / 2, sh - 52, fs, Fade(RAYWHITE, 0.88f));
        UI_DrawHUD(game->ui, sw, sh, game->objectiveText, false);
        UI_DrawFadeOverlay(game->ui, sw, sh);
        return;
    }

    if (game->state == GAME_STATE_TITLE) {
        UI_DrawTitleScreen(game->ui, game->screenWidth, game->screenHeight, game->stateTime);
        Game_DrawDevMenu(game);
        UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
        return;
    }

    if ((game->state == GAME_STATE_ENDING_1 && game->ending1Triggered)
        || (game->state == GAME_STATE_ENDING_2 && game->ending2Triggered)
        || (game->state == GAME_STATE_ENDING_4 && game->ending4Triggered)) {
        Color bg = (Color){ 20, 0, 0, 255 };
        const char *banner = "THE END";
        Color bannerCol = (Color){ 200, 80, 80, 255 };
        if (game->state == GAME_STATE_ENDING_2) {
            bg = (Color){ 8, 10, 26, 255 };
            banner = "ENDING 2";
            bannerCol = (Color){ 140, 180, 255, 255 };
        } else if (game->state == GAME_STATE_ENDING_4) {
            bg = (Color){ 8, 22, 16, 255 };
            banner = "ENDING 4";
            bannerCol = (Color){ 160, 230, 190, 255 };
        }
        ClearBackground(bg);
        int sz = 42;
        int tw = MeasureText(banner, sz);
        DrawText(banner, game->screenWidth / 2 - tw / 2, game->screenHeight / 2 - 40, sz, bannerCol);
        DrawText("Press Enter after the text, then advance to title",
                 game->screenWidth / 2 - MeasureText("Press Enter after the text, then advance to title", 14) / 2,
                 game->screenHeight / 2 + 12, 14, (Color){ 180, 180, 200, 255 });
        Dialogue_Draw(game->dialogue, game->screenWidth, game->screenHeight);
        Game_DrawDevMenu(game);
        UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
        return;
    }

    BeginMode2D(game->camera);
    Map_DrawBackground(game->map, game->player->position.y);
    {
        bool drawK = Game_ShouldDrawDay4Killer(game);
        bool playerSouthOfKiller = drawK && game->player->position.y > Day4_KillerSortYWorld();
        if (playerSouthOfKiller)
            Game_DrawDay4Killer(game);
        if (!game->hidingInLocker)
            Player_Draw(game->player);
        if (drawK && !playerSouthOfKiller)
            Game_DrawDay4Killer(game);
    }
    Map_DrawForeground(game->map, game->player->position.y);

    EndMode2D();

    if (game->activeMinigame != MINIGAME_NONE) {
        int sw = game->screenWidth;
        int sh = game->screenHeight;
        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 200 });
        Minigames_Draw(sw, sh);
    } else if (game->lightsOff) {
        /*
         * Day 4 lights-out: radial vignette — transparent at center, smooth smoothstep darkening to rOuter,
         * then solid black to the screen edge. Same radii as before (inner reference = character height).
         */
        int sw = game->screenWidth;
        int sh = game->screenHeight;
        Vector2 p = GetWorldToScreen2D(game->player->position, game->camera);
        float charH = game->player->size.y * game->camera.zoom;
        float rInner = charH * 0.5f;
        const float innerFrac = 0.30f;
        float rOuter = rInner / innerFrac;
        float d0 = hypotf(p.x, p.y);
        float d1 = hypotf((float)sw - p.x, p.y);
        float d2 = hypotf(p.x, (float)sh - p.y);
        float d3 = hypotf((float)sw - p.x, (float)sh - p.y);
        float maxD = fmaxf(fmaxf(d0, d1), fmaxf(d2, d3));
        float cover = maxD + 120.0f;
        const int fadeBands = 56;
        const float rMin = 0.5f;
        int segs = 96;
        for (int i = 0; i < fadeBands; i++) {
            float t0 = (float)i / (float)fadeBands;
            float t1 = (float)(i + 1) / (float)fadeBands;
            float rad0 = rMin + (rOuter - rMin) * t0;
            float rad1 = rMin + (rOuter - rMin) * t1;
            if (rad1 <= rad0) continue;
            float tm = (t0 + t1) * 0.5f;
            float u = tm * tm * (3.0f - 2.0f * tm); /* smoothstep: gentle start/end */
            unsigned char a = (unsigned char)(255.0f * u + 0.5f);
            DrawRing(p, rad0, rad1, 0.0f, 360.0f, segs, (Color){ 0, 0, 0, a });
        }
        DrawRing(p, rOuter, cover, 0.0f, 360.0f, segs, (Color){ 0, 0, 0, 255 });
    }

    if (game->activeMinigame == MINIGAME_NONE)
        Anomaly_DrawOverlay(game->anomalies);

    if (game->state == GAME_STATE_DAY4_STAIRS_CHOICE && !Dialogue_IsActive(game->dialogue)
        && game->activeMinigame == MINIGAME_NONE) {
        int sw = game->screenWidth;
        int sh = game->screenHeight;
        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 150 });
        int fs = 20;
        int y = sh / 2 - 72;
        const char *a = "[1] HIDE — basement locker (Ending 1)";
        const char *b = "[2] FIND A WEAPON — fight back (Ending 2)";
        const char *c = "[3] SHOUT HELLO — bluff the intruder (Ending 4)";
        DrawText(a, sw / 2 - MeasureText(a, fs) / 2, y, fs, (Color){ 255, 255, 255, 255 });
        DrawText(b, sw / 2 - MeasureText(b, fs) / 2, y + 32, fs, (Color){ 220, 235, 255, 255 });
        DrawText(c, sw / 2 - MeasureText(c, fs) / 2, y + 64, fs, (Color){ 220, 255, 220, 255 });
    }

    // Code created by wu deguang - on-screen "Press E to ..." prompt when player is in an interactable's trigger zone
    if (!Dialogue_IsActive(game->dialogue) && Game_IsPlayState(game->state) && game->activeMinigame == MINIGAME_NONE) {
        Rectangle playerRect = Player_GetBounds(game->player);
        const char *prompt = NULL;
        int n = 0;
        const Interactable *interactables = Map_GetInteractables(game->map, &n);
        for (int i = 0; i < n; i++) {
            Rectangle useZone = (interactables[i].triggerZone.width > 0 && interactables[i].triggerZone.height > 0) ? interactables[i].triggerZone : interactables[i].bounds;
            if (CheckCollisionRecs(playerRect, useZone)) {
                prompt = Game_GetInteractPrompt(game, &interactables[i]);
                break;
            }
        }
        if (!prompt && game->map->cashierBounds.width > 0 && CheckCollisionRecs(playerRect, game->map->cashierBounds))
            prompt = Game_GetCashierPrompt(game);
        if (prompt) {
            int pw = MeasureText(prompt, 18);
            DrawRectangle(game->screenWidth/2 - pw/2 - 12, game->screenHeight - 116, pw + 24, 32, (Color){ 10, 10, 20, 230 });
            DrawText(prompt, game->screenWidth/2 - pw/2, game->screenHeight - 110, 18, (Color){ 240, 235, 255, 255 });
        }
    }

    UI_DrawHUD(game->ui, game->screenWidth, game->screenHeight, game->objectiveText, game->paused);
    Dialogue_Draw(game->dialogue, game->screenWidth, game->screenHeight);
    Game_DrawDevMenu(game);
    UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
}
