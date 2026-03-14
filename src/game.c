#include "game.h"
#include "player.h"
#include "map.h"
#include "dialogue.h"
#include "anomaly.h"
#include "audio.h"
#include "ui.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>

static void Game_ChangeState(Game *game, GameState newState);
static void Game_HandleInput(Game *game, float dt);
static void Game_HandleInteractions(Game *game);
static void Game_UpdateObjective(Game *game);
static void Game_AdvanceDayIfReady(Game *game);
static bool Game_IsPlayState(GameState s);

// Paul spawn: kitchen backdoor area
static const float SPAWN_DAY1_X = 120.0f;
static const float SPAWN_DAY1_Y = 100.0f;
static const float SPAWN_CASHIER_X = 420.0f;
static const float SPAWN_BASEMENT_X = 450.0f;
static const float SPAWN_BASEMENT_Y = 460.0f;

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

    g->paused = false;
    g->requestRestart = false;
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

    g->objectiveText = "Press Enter to begin.";

    Game_ChangeState(g, GAME_STATE_TITLE);
    return g;
}

void Game_Destroy(Game *game) {
    if (!game) return;
    Anomaly_Destroy(game->anomalies);
    UI_Destroy(game->ui);
    Audio_Destroy(game->audio);
    Dialogue_Destroy(game->dialogue);
    Player_Destroy(game->player);
    Map_Destroy(game->map);
    free(game);
}

void Game_Restart(Game *game) {
    if (!game) return;
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
    game->objectiveText = "Press Enter to begin.";
    Game_ChangeState(game, GAME_STATE_TITLE);
}

static bool Game_IsPlayState(GameState s) {
    return s == GAME_STATE_DAY_1 || s == GAME_STATE_DAY_2 || s == GAME_STATE_DAY_3 || s == GAME_STATE_DAY_4
        || s == GAME_STATE_SCRIPTED_BLACKOUT || s == GAME_STATE_SCRIPTED_BASEMENT_CHASE;
}

static void Game_ChangeState(Game *game, GameState newState) {
    game->state = newState;
    game->stateTime = 0.0f;

    if (newState == GAME_STATE_INTRO) {
        Dialogue_Start(game->dialogue, DIALOGUE_INTRO_DAY1, game);
        UI_StartFade(game->ui, 0.0f, 0.6f);
    } else if (newState == GAME_STATE_DAY_1) {
        game->player->position.x = SPAWN_DAY1_X;
        game->player->position.y = SPAWN_DAY1_Y;
    } else if (newState == GAME_STATE_DAY_2) {
        game->player->position.x = SPAWN_DAY1_X;
        game->player->position.y = SPAWN_DAY1_Y;
        game->clockedIn = false;
        game->radioOn = false;
        game->dishesDone = false;
        game->moppingDone = false;
        game->garbageDone = false;
    } else if (newState == GAME_STATE_DAY_3) {
        game->player->position.x = SPAWN_DAY1_X;
        game->player->position.y = SPAWN_DAY1_Y;
        game->clockedIn = false;
        game->radioOn = false;
        game->dishesDone = false;
        game->moppingDone = false;
        game->garbageDone = false;
    } else if (newState == GAME_STATE_DAY_4) {
        game->map->bloodMoon = true;
        game->player->position.x = SPAWN_DAY1_X;
        game->player->position.y = SPAWN_DAY1_Y;
        game->clockedIn = false;
    } else if (newState == GAME_STATE_SCRIPTED_BLACKOUT) {
        game->lightsOff = true;
        Dialogue_Start(game->dialogue, DIALOGUE_DAY4_LIGHTS_OUT, game);
    } else if (newState == GAME_STATE_ENDING_1) {
        game->ending1Triggered = true;
        Dialogue_Start(game->dialogue, DIALOGUE_ENDING_1, game);
    }
}

static void Game_HandleInput(Game *game, float dt) {
    (void)dt;

    if (IsKeyPressed(KEY_R)) {
        game->requestRestart = true;
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

    if (game->state == GAME_STATE_ENDING_1) {
        if (IsKeyPressed(KEY_ENTER) && !Dialogue_IsActive(game->dialogue)) {
            Dialogue_Start(game->dialogue, DIALOGUE_THE_END, game);
        }
        if (Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_THE_END) {
            Game_ChangeState(game, GAME_STATE_TITLE);
        }
        if (Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_ENDING_1) {
            Dialogue_Start(game->dialogue, DIALOGUE_THE_END, game);
        }
        return;
    }
}

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

static void Game_HandleInteractions(Game *game) {
    if (!game || !game->player || !game->map) return;
    if (Dialogue_IsActive(game->dialogue)) return;

    if (!IsKeyPressed(KEY_E)) return;

    Rectangle playerRect = Player_GetBounds(game->player);
    int n = 0;
    const Interactable *interactables = Map_GetInteractables(game->map, &n);

    for (int i = 0; i < n; i++) {
        const Interactable *in = &interactables[i];
        if (!CheckCollisionRecs(playerRect, in->bounds)) continue;

        switch (in->type) {
            case INTERACT_BADGE:
                if (!game->clockedIn) {
                    game->clockedIn = true;
                    if (game->currentDay == 1)
                        Dialogue_Start(game->dialogue, DIALOGUE_DAY1_CLOCKED_IN, game);
                }
                return;
            case INTERACT_CLOCK_OUT:
                if (game->currentDay == 1 && game->clockedIn && game->moppingDone && game->dishesDone && game->garbageDone) {
                    Game_AdvanceDayIfReady(game);
                }
                return;
            case INTERACT_SINK:
                if (!game->dishesDone) {
                    game->dishesDone = true;
                }
                return;
            case INTERACT_MOP:
                if (!game->moppingDone) {
                    game->moppingDone = true;
                }
                return;
            case INTERACT_RADIO:
                if (!game->radioOn) {
                    game->radioOn = true;
                    if (game->currentDay == 2 && !game->day2RadioHeard) {
                        game->day2RadioHeard = true;
                        Dialogue_Start(game->dialogue, DIALOGUE_DAY2_RADIO_NEWS, game);
                    }
                }
                return;
            case INTERACT_GARBAGE:
                if (!game->garbageDone) game->garbageDone = true;
                return;
            case INTERACT_FREEZER_DOOR:
                if (game->currentDay == 3 && !game->day3FreezerDone) {
                    game->day3FreezerDone = true;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY3_FREEZER_DONE, game);
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
                    game->player->position.y = 380.0f;
                    game->inBasement = false;
                    game->day4FootstepsStarted = true;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY4_FOOTSTEPS, game);
                }
                return;
            case INTERACT_GENERATOR:
                if (game->inBasement && !game->generatorFixed) {
                    game->generatorFixed = true;
                    game->lightsOff = false;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY4_GENERATOR_FIXED, game);
                }
                return;
            case INTERACT_LOCKER_1:
            case INTERACT_LOCKER_2:
            case INTERACT_LOCKER_3:
                if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && !game->hidingInLocker) {
                    game->hidingInLocker = true;
                    Dialogue_Start(game->dialogue, DIALOGUE_DAY4_HIDE_IN_LOCKER, game);
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
        Dialogue_Start(game->dialogue, DIALOGUE_DAY4_SHAMAN, game);
        return;
    }
    (void)npcs;
}

static void Game_UpdateObjective(Game *game) {
    if (game->state == GAME_STATE_TITLE) {
        game->objectiveText = "Press Enter to begin.";
        return;
    }
    if (game->state == GAME_STATE_INTRO) {
        game->objectiveText = "Listen to the intro.";
        return;
    }
    if (game->state == GAME_STATE_ENDING_1) {
        game->objectiveText = "THE END. Press Enter to return to title.";
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
        else if (!game->lightsOff) game->objectiveText = "Day 4: Lights will go out...";
        else if (!game->generatorFixed) game->objectiveText = "Day 4: Go to basement and fix the generator.";
        else if (!game->day4FootstepsStarted) game->objectiveText = "Day 4: Go back up the stairs.";
        else if (!game->hidingInLocker) game->objectiveText = "Day 4: Hide in a locker!";
        else game->objectiveText = "Day 4: ...";
        return;
    }
}

void Game_Update(Game *game, float dt) {
    if (!game) return;

    game->stateTime += dt;
    Game_HandleInput(game, dt);

    if (game->requestRestart) {
        Game_Restart(game);
        return;
    }

    bool allowWorldUpdate = !game->paused;
    UI_Update(game->ui, dt);
    Dialogue_Update(game->dialogue);
    Audio_Update(game->audio);

    if (game->state == GAME_STATE_INTRO) {
        if (!Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue)) {
            Game_ChangeState(game, GAME_STATE_DAY_1);
        }
        Game_UpdateObjective(game);
        return;
    }

    if (Game_IsPlayState(game->state)) {
        if (allowWorldUpdate) {
            Player_Update(game->player, game->map, dt, !Dialogue_IsActive(game->dialogue));
            game->inBasement = (Map_GetRegionAt(game->map, game->player->position.x, game->player->position.y) == REGION_BASEMENT);

            if (game->currentDay == 3 && !game->day3BossCallHeard && game->stateTime > 2.0f) {
                game->day3BossCallHeard = true;
                Dialogue_Start(game->dialogue, DIALOGUE_DAY3_BOSS_CALL, game);
            }

            if (game->currentDay == 4 && game->day4ShamanLeft && !game->lightsOff && Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_DAY4_SHAMAN) {
                Game_ChangeState(game, GAME_STATE_SCRIPTED_BLACKOUT);
            }

            if (game->state == GAME_STATE_SCRIPTED_BLACKOUT && !Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_DAY4_LIGHTS_OUT) {
                game->state = GAME_STATE_DAY_4;
            }

            if (game->day4FootstepsStarted && !Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_DAY4_FOOTSTEPS) {
                Game_ChangeState(game, GAME_STATE_SCRIPTED_BASEMENT_CHASE);
            }

            if (game->state == GAME_STATE_SCRIPTED_BASEMENT_CHASE && game->hidingInLocker && !Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue) && game->dialogue->lastClosedId == DIALOGUE_DAY4_HIDE_IN_LOCKER) {
                Game_ChangeState(game, GAME_STATE_ENDING_1);
            }

            Game_HandleInteractions(game);
            Game_AdvanceDayIfReady(game);

            if (game->currentDay == 2 && game->state == GAME_STATE_DAY_1 && !Dialogue_IsActive(game->dialogue)) {
                Game_ChangeState(game, GAME_STATE_DAY_2);
            }
            if (game->currentDay == 3 && game->state == GAME_STATE_DAY_2 && !Dialogue_IsActive(game->dialogue)) {
                Game_ChangeState(game, GAME_STATE_DAY_3);
            }
            if (game->currentDay == 4 && game->state == GAME_STATE_DAY_3 && !Dialogue_IsActive(game->dialogue)) {
                Game_ChangeState(game, GAME_STATE_DAY_4);
            }
        }
        Anomaly_Update(game->anomalies, game, dt);
    }

    Game_UpdateObjective(game);
}

void Game_Draw(Game *game) {
    if (!game) return;

    if (game->state == GAME_STATE_TITLE) {
        UI_DrawTitleScreen(game->ui, game->screenWidth, game->screenHeight, game->stateTime);
        UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
        return;
    }

    if (game->state == GAME_STATE_ENDING_1 && game->ending1Triggered) {
        ClearBackground((Color){ 20, 0, 0, 255 });
        const char *txt = "THE END";
        int sz = 48;
        int tw = MeasureText(txt, sz);
        DrawText(txt, game->screenWidth/2 - tw/2, game->screenHeight/2 - 30, sz, (Color){ 200, 80, 80, 255 });
        DrawText("Press Enter to return to title",  game->screenWidth/2 - MeasureText("Press Enter to return to title", 16)/2, game->screenHeight/2 + 30, 16, (Color){ 180, 180, 200, 255 });
        Dialogue_Draw(game->dialogue, game->screenWidth, game->screenHeight);
        UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
        return;
    }

    Map_DrawBackground(game->map, game);
    if (!game->hidingInLocker)
        Player_Draw(game->player);
    Map_DrawForeground(game->map);

    if (game->lightsOff) {
        DrawRectangle(0, 0, game->screenWidth, game->screenHeight, (Color){ 0, 0, 0, 220 });
        if (!game->inBasement) {
            DrawCircle((int)game->player->position.x, (int)game->player->position.y, 80.0f, (Color){ 15, 15, 25, 180 });
        }
    }

    Anomaly_DrawOverlay(game->anomalies);

    UI_DrawHUD(game->ui, game->screenWidth, game->screenHeight, game->objectiveText, game->paused);
    Dialogue_Draw(game->dialogue, game->screenWidth, game->screenHeight);
    UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
}
