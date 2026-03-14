 #ifndef DIALOGUE_H
 #define DIALOGUE_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 typedef struct DialogueLine {
     const char *speaker;
     const char *text;
 } DialogueLine;
 
 // A simple dialogue script
 typedef struct DialogueScript {
     const DialogueLine *lines;
     int lineCount;
 } DialogueScript;
 
 typedef enum DialogueId {
     DIALOGUE_NONE = 0,
     DIALOGUE_INTRO_MONOLOGUE,
     DIALOGUE_VENDOR_FIRST,
     DIALOGUE_VENDOR_AFTER_ANOMALY,
     DIALOGUE_MASKED_CUSTOMER,
     DIALOGUE_CHILD_WHISPER_FIRST,
     DIALOGUE_CHILD_WHISPER_AFTER,
     DIALOGUE_FAKE_ANNOUNCEMENT,
     DIALOGUE_ENDING
 } DialogueId;
 
 typedef struct DialogueSystem {
     const DialogueScript *current;
     DialogueId currentId;
     int currentIndex;
     bool active;
     bool justClosed;
 } DialogueSystem;
 
 typedef struct Game Game;
 
 // Create / destroy
 DialogueSystem *Dialogue_Create(void);
 void            Dialogue_Destroy(DialogueSystem *dlg);
 
 // Per-frame
 void            Dialogue_Update(DialogueSystem *dlg);
 void            Dialogue_Draw(const DialogueSystem *dlg, int screenWidth, int screenHeight);
 
 // Control
 void            Dialogue_Start(DialogueSystem *dlg, DialogueId id, const Game *game);
 void            Dialogue_Close(DialogueSystem *dlg);
 
 // Helpers
 bool            Dialogue_IsActive(const DialogueSystem *dlg);
 bool            Dialogue_JustClosed(DialogueSystem *dlg);
 
 #endif // DIALOGUE_H
