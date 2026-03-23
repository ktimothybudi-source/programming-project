/*
 * ============================================================================
 * dialogue.c — ALL STORY TEXT AS DATA + SIMPLE PLAYER CONTROLS
 * ============================================================================
 *
 * HOW IT WORKS (read this first):
 *   1) We store story as static arrays: INTRO_DAY1[], DAY2_RADIO_NEWS[], ...
 *   2) SCRIPTS[] is a lookup table: SCRIPTS[DialogueId] = { pointer, line count }
 *   3) Dialogue_Start selects a script and sets currentIndex = 0
 *   4) Each time the player presses Space or Enter, we increment currentIndex
 *   5) When currentIndex >= lineCount, we close dialogue and set justClosed = true
 *
 * GetScriptForId converts DialogueId to an index into SCRIPTS[] (must stay in sync
 * with the enum order in dialogue.h).
 * ============================================================================
 */

#include "dialogue.h"
#include "game.h"
#include "raylib.h"
#include <stdlib.h>

/* ========================= Story content (Day 1) ========================= */
static const DialogueLine INTRO_DAY1[] = {
    { "Narrator", "There lives an eighteen year old student named Paul\nwho studies at Beijing Institute of Technology." },
    { "Narrator", "Paul decided to apply for a job at Bohou's supermarket.\nHe got accepted for night shifts." },
    { "Narrator", "This is where the journey of horror will start." }
};

static const DialogueLine DAY1_CLOCKED_IN[] = {
    { "Paul", "First day. I'll swipe my badge and get started." },
    { "Narrator", "Paul swipes his ID card on the badge reader.\nHe is alone; no customers yet, no colleagues." }
};

static const DialogueLine DAY1_FIRST_CUSTOMERS[] = {
    { "Narrator", "In the first 30 minutes Paul handled a few shoppers.\nHe decided to use the time to do supermarket chores." }
};

static const DialogueLine DAY1_END_OF_DAY[] = {
    { "Paul", "Mopping done, dishes done, garbage out.\nTime to clock out and go home." },
    { "Narrator", "Paul closes the supermarket and goes back to his dormitory." }
};

/* ========================= Story content (Day 2) ========================= */
static const DialogueLine DAY2_RADIO_NEWS[] = {
    { "Radio", "Good evening everyone. Our top story tonight:\na well-known serial killer who calls himself Mike Hawk." },
    { "Radio", "He is currently on the run after murdering a family of four.\nHe was last seen near the lake of BeiHu." },
    { "Radio", "He appears to be 5'6, tanned skin, black hair,\na curly mustache. If you are near this area, please be careful." }
};

static const DialogueLine DAY2_YOUNG_LADY[] = {
    { "Young Lady", "Hey, have you heard about the serial killer going around BeiHu?\nIsn't that terrifying?!" },
    { "Paul", "Hahaha, you believe in that joke news? It isn't real!" },
    { "Young Lady", "News wouldn't give hoax news! You be safe then, goodbye." }
};

static const DialogueLine DAY2_OLD_MAN[] = {
    { "Old Man", "BE CAREFUL! HE IS WATCHING YOU!\nBE AWARE OF WHAT GOES AROUND THIS PLACE!" },
    { "Paul", "(This old man is crazy... I'll just finish and say goodbye.)" },
    { "Old Man", "In 2 more days, be EXTREMELY CAUTIOUS." },
    { "Narrator", "The old man held Paul's hands. Paul brushed him off and called him crazy." }
};

static const DialogueLine DAY2_END[] = {
    { "Paul", "Finally done. Everyone's going crazy over fake news.\nI'm clocking out." }
};

/* ========================= Story content (Day 3) ========================= */
static const DialogueLine DAY3_BOSS_CALL[] = {
    { "Boss", "Hello Paul, please take out the expired meat in the freezer\nand throw it in the garbage, thanks!" },
    { "Boss", "Oh yeah, and be careful. I heard there was a serial killer\nwandering around BeiHu yesterday. Please be cautious!" },
    { "Paul", "Boss, how do you even fall for that fake news?\nYou should know better." },
    { "Boss", "Kid, I'm just telling you to be careful alright? Well then, goodbye." }
};

static const DialogueLine DAY3_FREEZER_DONE[] = {
    { "Narrator", "Paul stepped into the freezer. A large volume of cold air sprayed onto his face.\nHe found the expired meat, wrapped it in a black bag, and threw it in the bin." }
};

static const DialogueLine DAY3_TEEN_BOY[] = {
    { "Teen Boy", "Yo! This night is awfully quiet which is weird.\nThe fog is getting thicker. You should be careful, stay safe." },
    { "Paul", "Okay dude, thanks! Have a great night." }
};

static const DialogueLine DAY3_OLD_LADY[] = {
    { "Old Lady", "Oh my gosh! I saw the killer lurking near BoHou!\nPlease hurry up and process my things!" },
    { "Paul", "Ma'am, there is no killer. I think you are delusional.\nBut go ahead I guess." },
    { "Narrator", "She packed her things and ran off." }
};

static const DialogueLine DAY3_CREEPY_MAN[] = {
    { "Mysterious Man", "DoEs ThIs PlAcE sElL hUmAn TeEtH aNd MaGgOtS?" },
    { "Paul", "Uh, sorry sir, but the hospital is on the west side of BoHou.\nIt's free there if you were wondering!" },
    { "Mysterious Man", "YOU FOOL! SEE WHAT I'LL DO TO YOU TOMORROW!" },
    { "Narrator", "The man dashed out of the store. Paul thought: What a crazy guy." }
};

static const DialogueLine DAY3_END[] = {
    { "Paul", "Very sleepy. Time to clock out and go home." }
};

/* ========================= Story content (Day 4 + ending) ========================= */
static const DialogueLine DAY4_SHAMAN[] = {
    { "Shaman", "Your future can be in shambles." },
    { "Narrator", "The old lady dressed like a shaman didn't place anything on the desk.\nShe left through the exit." }
};

static const DialogueLine DAY4_LIGHTS_OUT[] = {
    { "Narrator", "All the lights suddenly cut off. It was pitch dark.\nPaul reached for his phone and turned on the flashlight." },
    { "Paul", "Something's wrong with the wires.\nThe generator is in the basement." }
};

static const DialogueLine DAY4_GENERATOR_FIXED[] = {
    { "Paul", "Fixed it. Lights are back." },
    { "Paul", "For a second I thought the killer sabotaged the lights\nand would get me right then and there." }
};

static const DialogueLine DAY4_FOOTSTEPS[] = {
    { "Narrator", "Heavy rapid footsteps suddenly rush toward the basement." },
    { "Paul", "(I have to hide. Now.)" }
};

static const DialogueLine DAY4_HIDE_IN_LOCKER[] = {
    { "Narrator", "Paul rushed to hide inside the corner locker.\nHis breath was loud; he tried to tone it down." },
    { "Narrator", "Through the gaps he saw a short, tanned guy with a curly mustache.\nThe crazy man from yesterday. Mike Hawk." },
    { "Narrator", "The killer inspected the basement. He walked toward the lockers.\nHe didn't bother to open them - they seemed locked." },
    { "Narrator", "Then the locker door was struck with the machete. BANG.\nThe door opened." }
};

static const DialogueLine ENDING_1[] = {
    { "Narrator", "Paul begged for mercy. Mid-sentence, Paul was beheaded by the murderer.\nBlood squirted from his neck. Mike Hawk fled." },
    { "Narrator", "THE END" }
};

static const DialogueLine THE_END[] = {
    { "", "THE END" }
};

/*
 * SCRIPTS[] must have the SAME ORDER as enum DialogueId in dialogue.h.
 * Index 0 = DIALOGUE_NONE (empty). sizeof(array)/sizeof(array[0]) counts lines.
 */
static const DialogueScript SCRIPTS[] = {
    { NULL, 0 },
    { INTRO_DAY1, sizeof(INTRO_DAY1)/sizeof(INTRO_DAY1[0]) },
    { DAY1_CLOCKED_IN, sizeof(DAY1_CLOCKED_IN)/sizeof(DAY1_CLOCKED_IN[0]) },
    { DAY1_FIRST_CUSTOMERS, sizeof(DAY1_FIRST_CUSTOMERS)/sizeof(DAY1_FIRST_CUSTOMERS[0]) },
    { DAY1_END_OF_DAY, sizeof(DAY1_END_OF_DAY)/sizeof(DAY1_END_OF_DAY[0]) },
    { DAY2_RADIO_NEWS, sizeof(DAY2_RADIO_NEWS)/sizeof(DAY2_RADIO_NEWS[0]) },
    { DAY2_YOUNG_LADY, sizeof(DAY2_YOUNG_LADY)/sizeof(DAY2_YOUNG_LADY[0]) },
    { DAY2_OLD_MAN, sizeof(DAY2_OLD_MAN)/sizeof(DAY2_OLD_MAN[0]) },
    { DAY2_END, sizeof(DAY2_END)/sizeof(DAY2_END[0]) },
    { DAY3_BOSS_CALL, sizeof(DAY3_BOSS_CALL)/sizeof(DAY3_BOSS_CALL[0]) },
    { DAY3_FREEZER_DONE, sizeof(DAY3_FREEZER_DONE)/sizeof(DAY3_FREEZER_DONE[0]) },
    { DAY3_TEEN_BOY, sizeof(DAY3_TEEN_BOY)/sizeof(DAY3_TEEN_BOY[0]) },
    { DAY3_OLD_LADY, sizeof(DAY3_OLD_LADY)/sizeof(DAY3_OLD_LADY[0]) },
    { DAY3_CREEPY_MAN, sizeof(DAY3_CREEPY_MAN)/sizeof(DAY3_CREEPY_MAN[0]) },
    { DAY3_END, sizeof(DAY3_END)/sizeof(DAY3_END[0]) },
    { DAY4_SHAMAN, sizeof(DAY4_SHAMAN)/sizeof(DAY4_SHAMAN[0]) },
    { DAY4_LIGHTS_OUT, sizeof(DAY4_LIGHTS_OUT)/sizeof(DAY4_LIGHTS_OUT[0]) },
    { DAY4_GENERATOR_FIXED, sizeof(DAY4_GENERATOR_FIXED)/sizeof(DAY4_GENERATOR_FIXED[0]) },
    { DAY4_FOOTSTEPS, sizeof(DAY4_FOOTSTEPS)/sizeof(DAY4_FOOTSTEPS[0]) },
    { DAY4_HIDE_IN_LOCKER, sizeof(DAY4_HIDE_IN_LOCKER)/sizeof(DAY4_HIDE_IN_LOCKER[0]) },
    { ENDING_1, sizeof(ENDING_1)/sizeof(ENDING_1[0]) },
    { THE_END, sizeof(THE_END)/sizeof(THE_END[0]) }
};

/* Returns pointer into SCRIPTS[] or NULL if id is out of range. */
static const DialogueScript *GetScriptForId(DialogueId id, const Game *game) {
    (void)game;
    int idx = (int)id;
    if (idx <= 0 || idx >= (int)(sizeof(SCRIPTS)/sizeof(SCRIPTS[0]))) return NULL;
    return &SCRIPTS[idx];
}

DialogueSystem *Dialogue_Create(void) {
    DialogueSystem *d = (DialogueSystem *)malloc(sizeof(DialogueSystem));
    if (!d) return NULL;
    d->current = NULL;
    d->currentId = DIALOGUE_NONE;
    d->lastClosedId = DIALOGUE_NONE;
    d->currentIndex = 0;
    d->active = false;
    d->justClosed = false;
    return d;
}

void Dialogue_Destroy(DialogueSystem *dlg) {
    if (dlg) free(dlg);
}

/*
 * Each frame: clear the "just closed" flag from last frame.
 * If dialogue is active and player pressed Space/Enter, go to next line or close.
 */
void Dialogue_Update(DialogueSystem *dlg) {
    dlg->justClosed = false;
    if (!dlg->active || !dlg->current) return;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        dlg->currentIndex++;
        if (dlg->currentIndex >= dlg->current->lineCount) {
            dlg->lastClosedId = dlg->currentId;
            dlg->active = false;
            dlg->current = NULL;
            dlg->currentId = DIALOGUE_NONE;
            dlg->justClosed = true;
        }
    }
}

/* Draw one rounded rectangle at bottom of screen + speaker + line + hint text. */
void Dialogue_Draw(const DialogueSystem *dlg, int screenWidth, int screenHeight) {
    if (!dlg->active || !dlg->current) return;

    const DialogueLine *line = &dlg->current->lines[dlg->currentIndex];

    int boxHeight = 140;
    Rectangle box = { 32.0f, (float)screenHeight - boxHeight - 32.0f,
                      (float)screenWidth - 64.0f, (float)boxHeight };

    DrawRectangleRounded(box, 0.08f, 10, (Color){ 8, 8, 16, 220 });
    DrawRectangleLinesEx(box, 2.0f, (Color){ 190, 190, 220, 255 });

    int padding = 16;
    if (line->speaker && line->speaker[0]) {
        DrawText(line->speaker, (int)(box.x + padding),
                 (int)(box.y + padding - 6), 18, (Color){ 240, 230, 250, 255 });
    }

    int textX = (int)(box.x + padding);
    int textY = (int)(box.y + padding + 16);
    DrawText(line->text, textX, textY, 16, (Color){ 230, 230, 240, 255 });

    const char *hint = "[Space / Enter] Continue";
    int hintWidth = MeasureText(hint, 12);
    DrawText(hint, (int)(box.x + box.width - hintWidth - 18),
             (int)(box.y + box.height - 22), 12, (Color){ 180, 180, 200, 255 });
}

/* Load script by id, reset to first line, mark active. game is unused here but kept for API consistency. */
void Dialogue_Start(DialogueSystem *dlg, DialogueId id, const Game *game) {
    const DialogueScript *s = GetScriptForId(id, game);
    if (!s || s->lineCount == 0) return;
    dlg->current = s;
    dlg->currentId = id;
    dlg->currentIndex = 0;
    dlg->active = true;
    dlg->justClosed = false;
}

void Dialogue_Close(DialogueSystem *dlg) {
    dlg->active = false;
    dlg->current = NULL;
    dlg->currentId = DIALOGUE_NONE;
    dlg->currentIndex = 0;
    dlg->justClosed = true;
}

bool Dialogue_IsActive(const DialogueSystem *dlg) {
    return dlg->active;
}

bool Dialogue_JustClosed(DialogueSystem *dlg) {
    return dlg->justClosed;
}
