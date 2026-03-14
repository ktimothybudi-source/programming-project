 #include "dialogue.h"
 #include "game.h"
 #include "raylib.h"
 #include <stdlib.h>
 
 // ---- Static scripts ----
 
 static const DialogueLine INTRO_LINES[] = {
     { "You", "BIT Anomalies Log 01.\nThe dean said this night market only exists\non certain Fridays." },
     { "You", "If rumors are true, people's memories\nloop and rewrite themselves here." },
     { "You", "I'll record anything... wrong." }
 };
 
 static const DialogueLine VENDOR_FIRST_LINES[] = {
     { "Vendor", "Skewers? Fresh... enough." },
     { "Vendor", "You look like a student.\nYou shouldn't stay after closing." },
     { "Vendor", "Last time, the lights blinked,\nand suddenly everyone remembered a\ndifferent rainstorm." }
 };
 
 static const DialogueLine VENDOR_AFTER_LINES[] = {
     { "Vendor", "Oh. You noticed it too." },
     { "Vendor", "The lanterns hum when the story bends." },
     { "Vendor", "Eat something, or the market will\nfill the emptiness with its own\nversion of you." }
 };
 
 static const DialogueLine MASKED_CUSTOMER_LINES[] = {
     { "Masked Customer", "... ..." },
     { "Masked Customer", "Don't worry. The mask helps.\nWhen the world shivers,\nI pretend I don't." },
     { "Masked Customer", "I think we met already.\nBut I'm sure this is your first time." }
 };
 
 static const DialogueLine CHILD_FIRST_LINES[] = {
     { "Child's Voice", "You stepped where the map is thin." },
     { "You", "(The alley is empty...)" },
     { "Child's Voice", "If you follow the wrong lantern,\nyou can leave before you arrive." }
 };
 
 static const DialogueLine CHILD_AFTER_LINES[] = {
     { "Child's Voice", "See? The stall moved.\nBut the owner remembers\nthe old spot." },
     { "Child's Voice", "When you're graded,\nwill they grade the you\nthat arrived or the you\nthat left?" }
 };
 
 static const DialogueLine FAKE_ANNOUNCE_LINES[] = {
     { "Announcement", "*PA system crackles to life,*\nthough there are no speakers." },
     { "Announcement", "Attention shoppers.\nThe rain you remember has been\ncorrected." },
     { "Announcement", "Please discard obsolete feelings\nat the nearest stall." }
 };
 
 static const DialogueLine ENDING_LINES[] = {
     { "You", "Three anomalies confirmed." },
     { "You", "Lantern color drift.\nSpatial stall displacement.\nConversation rewriting." },
     { "You", "The market edits us gently.\nLike a teacher correcting\nred pen across the sky." },
     { "You", "I'll file the report.\nIf I still remember\nhow the night really felt." }
 };
 
 static const DialogueScript SCRIPTS[] = {
     { NULL, 0 }, // NONE
     { INTRO_LINES, sizeof(INTRO_LINES)/sizeof(INTRO_LINES[0]) },
     { VENDOR_FIRST_LINES, sizeof(VENDOR_FIRST_LINES)/sizeof(VENDOR_FIRST_LINES[0]) },
     { VENDOR_AFTER_LINES, sizeof(VENDOR_AFTER_LINES)/sizeof(VENDOR_AFTER_LINES[0]) },
     { MASKED_CUSTOMER_LINES, sizeof(MASKED_CUSTOMER_LINES)/sizeof(MASKED_CUSTOMER_LINES[0]) },
     { CHILD_FIRST_LINES, sizeof(CHILD_FIRST_LINES)/sizeof(CHILD_FIRST_LINES[0]) },
     { CHILD_AFTER_LINES, sizeof(CHILD_AFTER_LINES)/sizeof(CHILD_AFTER_LINES[0]) },
     { FAKE_ANNOUNCE_LINES, sizeof(FAKE_ANNOUNCE_LINES)/sizeof(FAKE_ANNOUNCE_LINES[0]) },
     { ENDING_LINES, sizeof(ENDING_LINES)/sizeof(ENDING_LINES[0]) }
 };
 
 // ---- Implementation ----
 
 DialogueSystem *Dialogue_Create(void) {
     DialogueSystem *d = (DialogueSystem *)malloc(sizeof(DialogueSystem));
     if (!d) return NULL;
     d->current = NULL;
     d->currentId = DIALOGUE_NONE;
     d->currentIndex = 0;
     d->active = false;
     d->justClosed = false;
     return d;
 }
 
 void Dialogue_Destroy(DialogueSystem *dlg) {
     if (dlg) free(dlg);
 }
 
 void Dialogue_Update(DialogueSystem *dlg) {
     dlg->justClosed = false;
     if (!dlg->active || !dlg->current) return;
 
     if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
         dlg->currentIndex++;
         if (dlg->currentIndex >= dlg->current->lineCount) {
             dlg->active = false;
             dlg->current = NULL;
             dlg->currentId = DIALOGUE_NONE;
             dlg->justClosed = true;
         }
     }
 }
 
 void Dialogue_Draw(const DialogueSystem *dlg, int screenWidth, int screenHeight) {
     if (!dlg->active || !dlg->current) return;
 
     const DialogueLine *line = &dlg->current->lines[dlg->currentIndex];
 
     int boxHeight = 140;
     Rectangle box = { 32.0f, (float)screenHeight - boxHeight - 32.0f,
                       (float)screenWidth - 64.0f, (float)boxHeight };
 
     DrawRectangleRounded(box, 0.08f, 10, (Color){ 8, 8, 16, 220 });
     DrawRectangleLinesEx(box, 2.0f, (Color){ 190, 190, 220, 255 });
 
     // Speaker name
     int padding = 16;
     if (line->speaker) {
         DrawText(line->speaker, (int)(box.x + padding),
                  (int)(box.y + padding - 6), 18, (Color){ 240, 230, 250, 255 });
     }
 
     // Text (supports '\n')
     int textX = (int)(box.x + padding);
     int textY = (int)(box.y + padding + 16);
     DrawText(line->text, textX, textY, 16, (Color){ 230, 230, 240, 255 });
 
     const char *hint = "[Space / Enter] Continue";
     int hintWidth = MeasureText(hint, 12);
     DrawText(hint, (int)(box.x + box.width - hintWidth - 18),
              (int)(box.y + box.height - 22), 12, (Color){ 180, 180, 200, 255 });
 }
 
 static const DialogueScript *GetScriptForId(DialogueId id, const Game *game) {
     (void)game; // branching is handled in caller
     int idx = (int)id;
     if (idx < 0 || idx >= (int)(sizeof(SCRIPTS)/sizeof(SCRIPTS[0]))) return NULL;
     return &SCRIPTS[idx];
 }
 
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
