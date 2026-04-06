/*
 * ============================================================================
 * dialogue.h — IN-GAME TEXT BOXES (VISUAL NOVEL STYLE)
 * ============================================================================
 *
 * A "dialogue script" is an ARRAY of lines. Each line has:
 *   speaker — name shown at top of box (can be empty string)
 *   text    — body text (can include \n for newline)
 *
 * DialogueId is an ENUM: a fixed list of named numbers. DIALOGUE_NONE = 0.
 * dialogue.c contains a big table SCRIPTS[] where index matches DialogueId so
 * Dialogue_Start(game, SOME_ID) looks up the right array of lines.
 *
 * While active, the player usually cannot move; game.c checks Dialogue_IsActive.
 * ============================================================================
 */

#ifndef DIALOGUE_H
#define DIALOGUE_H

#include "raylib.h"
#include <stdbool.h>

typedef struct DialogueLine {
    const char *speaker;
    const char *text;
} DialogueLine;

typedef struct DialogueScript {
    const DialogueLine *lines;
    int lineCount;
} DialogueScript;

typedef enum DialogueId {
    DIALOGUE_NONE = 0,
    DIALOGUE_INTRO_DAY1,
    DIALOGUE_DAY1_CLOCKED_IN,
    DIALOGUE_DAY1_FIRST_CUSTOMERS,
    DIALOGUE_DAY1_END_OF_DAY,
    DIALOGUE_DAY2_RADIO_NEWS,
    DIALOGUE_DAY2_YOUNG_LADY,
    DIALOGUE_DAY2_OLD_MAN,
    DIALOGUE_DAY2_END,
    DIALOGUE_DAY3_BOSS_CALL,
    DIALOGUE_DAY3_FREEZER_DONE,
    DIALOGUE_DAY3_TEEN_BOY,
    DIALOGUE_DAY3_OLD_LADY,
    DIALOGUE_DAY3_CREEPY_MAN,
    DIALOGUE_DAY3_END,
    DIALOGUE_DAY4_SHAMAN,
    DIALOGUE_DAY4_LIGHTS_OUT,
    DIALOGUE_DAY4_GENERATOR_FIXED,
    DIALOGUE_DAY4_FOOTSTEPS,
    DIALOGUE_DAY4_HIDE_IN_LOCKER,
    DIALOGUE_ENDING_1,
    DIALOGUE_ENDING_2,
    DIALOGUE_ENDING_4,
    DIALOGUE_THE_END,
    DIALOGUE_COUNT
} DialogueId;

typedef struct DialogueSystem {
    const DialogueScript *current;
    DialogueId currentId;
    DialogueId lastClosedId;
    int currentIndex;
    bool active;
    bool justClosed;
    /* Typewriter: visibleChars = bytes shown of current line (ASCII scripts). */
    int visibleChars;
    float typewriterAcc;
} DialogueSystem;

typedef struct Game Game;

DialogueSystem *Dialogue_Create(void);
void            Dialogue_Destroy(DialogueSystem *dlg);
void            Dialogue_Update(DialogueSystem *dlg);
void            Dialogue_Draw(const DialogueSystem *dlg, int screenWidth, int screenHeight);
void            Dialogue_Start(DialogueSystem *dlg, DialogueId id, const Game *game);
void            Dialogue_Close(DialogueSystem *dlg);
bool            Dialogue_IsActive(const DialogueSystem *dlg);
bool            Dialogue_JustClosed(DialogueSystem *dlg);

#endif /* DIALOGUE_H */
