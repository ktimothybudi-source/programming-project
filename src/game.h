#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

// Forward declarations of modules
typedef struct Player Player;
typedef struct Map Map;
typedef struct DialogueSystem DialogueSystem;
typedef struct AnomalyManager AnomalyManager;
typedef struct AudioManager AudioManager;
typedef struct UIState UIState;

// Core game states: day-based story flow
typedef enum GameState {
    GAME_STATE_TITLE = 0,
    GAME_STATE_INTRO,           // Day 1 intro narration
    GAME_STATE_DAY_1,
    GAME_STATE_DAY_2,
    GAME_STATE_DAY_3,
    GAME_STATE_DAY_4,
    GAME_STATE_SCRIPTED_BLACKOUT,
    GAME_STATE_SCRIPTED_BASEMENT_CHASE,
    GAME_STATE_ENDING_1
} GameState;

// High-level game container
typedef struct Game {
    GameState state;
    float stateTime;
    bool paused;
    bool requestRestart;

    Map *map;
    Player *player;
    DialogueSystem *dialogue;
    AnomalyManager *anomalies;
    AudioManager *audio;
    UIState *ui;

    // Day and task progression
    int currentDay;             // 1-4
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

    // Day 2: customers served
    bool day2RadioHeard;
    bool day2YoungLadyServed;
    bool day2OldManServed;

    // Day 3: events
    bool day3BossCallHeard;
    bool day3FreezerDone;
    bool day3TeenBoyServed;
    bool day3OldLadyServed;
    bool day3CreepyManServed;

    // Day 4: events
    bool day4ShamanLeft;
    bool day4FootstepsStarted;
    bool day4KillerVisible;

    // Current objective text (set by Game_UpdateObjective)
    const char *objectiveText;

    // Cached window size
    int screenWidth;
    int screenHeight;
} Game;

// Lifecycle
Game *Game_Create(int screenWidth, int screenHeight);
void   Game_Destroy(Game *game);

// Update & draw
void   Game_Update(Game *game, float dt);
void   Game_Draw(Game *game);

// Utility
void   Game_Restart(Game *game);

#endif // GAME_H
