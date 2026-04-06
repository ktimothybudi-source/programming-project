/*
 * Embedded raylib minigames (card swipe, mop, dishes, radio, trash, freezer, generator).
 * Run inside the main window via a virtual framebuffer + letterboxing.
 */
#ifndef MINIGAMES_H
#define MINIGAMES_H

#include "raylib.h"
#include <stdbool.h>

typedef enum MinigameId {
    MINIGAME_NONE = 0,
    MINIGAME_CARD_SWIPE,
    MINIGAME_MOP,
    MINIGAME_DISHES,
    MINIGAME_RADIO,
    MINIGAME_TRASH,
    MINIGAME_FREEZER,
    MINIGAME_GENERATOR,
    MINIGAME_BOSS
} MinigameId;

typedef enum MinigameResult {
    MINIGAME_CONTINUE = 0,
    MINIGAME_WON,
    MINIGAME_CANCELLED,
    MINIGAME_LOST
} MinigameResult;

bool Minigames_Init(void);
void Minigames_Shutdown(void);

void Minigames_Start(MinigameId id);
MinigameId Minigames_GetActive(void);
void Minigames_Clear(void);

/* dt from GetFrameTime(); ESC cancels most minigames; boss: ESC surrenders (MINIGAME_LOST). */
MinigameResult Minigames_Update(float dt, int screenW, int screenH);
void Minigames_Draw(int screenW, int screenH);

#endif
