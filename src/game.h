/*
 * ============================================================================
 * game.h — THE MAIN "GAME OBJECT" AND PUBLIC FUNCTIONS
 * ============================================================================
 *
 * "typedef struct X X" means: the struct tag and the type name are both X.
 * Other headers can say "struct Game *" or "Game *" after including this.
 *
 * GameState tells us WHICH SCREEN OR MODE we are in (title, day 1, ending...).
 * The big struct Game holds EVERYTHING: pointers to map/player/dialogue AND
 * dozens of bools for "did the player finish task X yet".
 *
 * game.c implements Game_Create, Game_Update, Game_Draw, Game_Restart.
 * ============================================================================
 */

#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "minigames/minigames.h"
#include <stdbool.h>

typedef struct Player Player;
typedef struct Map Map;
typedef struct DialogueSystem DialogueSystem;
typedef struct AnomalyManager AnomalyManager;
typedef struct AudioManager AudioManager;
typedef struct UIState UIState;
struct IntroVideoPlayer;

/*
 * One enum value per major mode. The game is mostly a STATE MACHINE: we only
 * run certain code when state == GAME_STATE_DAY_1, etc.
 */
typedef enum GameState {
    GAME_STATE_TITLE = 0,
    GAME_STATE_INTRO,
    GAME_STATE_DAY_1,
    GAME_STATE_DAY_2,
    GAME_STATE_DAY_3,
    GAME_STATE_DAY_4,
    GAME_STATE_SCRIPTED_BLACKOUT,
    GAME_STATE_SCRIPTED_BASEMENT_CHASE,
    GAME_STATE_DAY4_STAIRS_CHOICE,
    GAME_STATE_ENDING_1,
    GAME_STATE_ENDING_2,
    GAME_STATE_ENDING_4
} GameState;

typedef struct Game {
    GameState state;
    float stateTime;
    bool paused;
    bool requestRestart;
    /* F: developer menu — jump to day 1–4 (testing). */
    bool devMenuOpen;

    Map *map;
    Player *player;
    DialogueSystem *dialogue;
    AnomalyManager *anomalies;
    AudioManager *audio;
    UIState *ui;

    int currentDay;
    bool clockedIn;
    bool radioOn;
    bool dishesDone;
    bool moppingDone;
    bool garbageDone;
    bool freezerDone;
    bool generatorFixed;
    bool lightsOff;
    bool inBasement;
    bool hidingInLocker;
    bool ending1Triggered;
    bool ending2Triggered;
    bool ending4Triggered;

    bool day2RadioHeard;
    bool day2YoungLadyServed;
    bool day2OldManServed;

    bool day3BossCallHeard;
    bool day3FreezerDone;
    bool day3TeenBoyServed;
    bool day3OldLadyServed;
    bool day3CreepyManServed;

    bool day4ShamanLeft;
    bool day4FootstepsStarted;
    bool day4KillerVisible;
    /* Day 4: Mike Hawk parry minigame cleared — stairs up unlocked. */
    bool day4BossDone;
    /* Set when shaman dialogue actually starts; blackout runs once it closes (reliable vs lastClosedId). */
    bool day4BlackoutAfterShaman;

    const char *objectiveText;

    int screenWidth;
    int screenHeight;

    /* Code created by wu deguang — 2D camera: offset = screen center, target = look-at point in world */
    Camera2D camera;

    MinigameId activeMinigame;
    /* Opening cinematic (assets/video/intro.mp4); NULL after skip or finish. */
    struct IntroVideoPlayer *introVideo;

    /* Day 4: Mike Hawk sprite on basement stairs after lights return (npc_mike_hawk.png). */
    Texture2D day4KillerSprite;
    bool day4KillerSpriteLoaded;
} Game;

Game *Game_Create(int screenWidth, int screenHeight);
void   Game_Destroy(Game *game);

void   Game_Update(Game *game, float dt);
void   Game_Draw(Game *game);

void   Game_Restart(Game *game);

#endif /* GAME_H */
