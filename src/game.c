 #include "game.h"
 #include "player.h"
 #include "map.h"
 #include "dialogue.h"
 #include "anomaly.h"
 #include "audio.h"
 #include "ui.h"
 #include "raylib.h"
 #include <stdlib.h>
 
 static void Game_ChangeState(Game *game, GameState newState);
 static void Game_HandleInput(Game *game, float dt);
 static void Game_HandleInteractions(Game *game);
 static void Game_UpdateObjective(Game *game);
 
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
 
     g->anomaly1Triggered = false;
     g->anomaly2Triggered = false;
     g->anomaly3Triggered = false;
     g->allAnomaliesResolved = false;
     g->objectiveText = "Begin investigation. Talk to the vendor.";
 
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
     game->anomaly1Triggered = false;
     game->anomaly2Triggered = false;
     game->anomaly3Triggered = false;
     game->allAnomaliesResolved = false;
     game->objectiveText = "Begin investigation. Talk to the vendor.";
 
     Game_ChangeState(game, GAME_STATE_TITLE);
 }
 
 static void Game_ChangeState(Game *game, GameState newState) {
     game->state = newState;
     game->stateTime = 0.0f;
 
     if (newState == GAME_STATE_INTRO) {
         Dialogue_Start(game->dialogue, DIALOGUE_INTRO_MONOLOGUE, game);
         UI_StartFade(game->ui, 0.0f, 0.6f);
     } else if (newState == GAME_STATE_ENDING) {
         Dialogue_Start(game->dialogue, DIALOGUE_ENDING, game);
         UI_StartFade(game->ui, 0.6f, 0.4f);
     }
 }
 
 static void Game_HandleInput(Game *game, float dt) {
     (void)dt;
 
     // Global restart
     if (IsKeyPressed(KEY_R)) {
         game->requestRestart = true;
         return;
     }
 
     // Pause toggle (not in title)
     if (IsKeyPressed(KEY_ESCAPE) && game->state != GAME_STATE_TITLE) {
         game->paused = !game->paused;
     }
 
     if (game->state == GAME_STATE_TITLE) {
         if (IsKeyPressed(KEY_ENTER)) {
             Game_ChangeState(game, GAME_STATE_INTRO);
         }
         return;
     }
 
     if (game->state == GAME_STATE_ENDING) {
         // Allow going back to title on Enter
         if (IsKeyPressed(KEY_ENTER) && !Dialogue_IsActive(game->dialogue)) {
             Game_ChangeState(game, GAME_STATE_TITLE);
         }
         return;
     }
 }
 
 static void Game_HandleInteractions(Game *game) {
     if (!game || !game->player || !game->map) return;
     if (Dialogue_IsActive(game->dialogue)) return;
 
     bool pressedInteract = IsKeyPressed(KEY_E);
     if (!pressedInteract) return;
 
     int stallCount = 0;
     const Stall *stalls = Map_GetStalls(game->map, &stallCount);
     int npcCount = 0;
     const NpcSpot *npcs = Map_GetNpcs(game->map, &npcCount);
 
     Rectangle playerRect = Player_GetBounds(game->player);
 
     // Prioritize NPCs
     for (int i = 0; i < npcCount; i++) {
         if (CheckCollisionRecs(playerRect, npcs[i].bounds)) {
             // Branching: child voice changes after anomalies
             if (npcs[i].label && npcs[i].label[0] == '?' ) {
                 if (game->anomaly3Triggered) {
                     Dialogue_Start(game->dialogue, DIALOGUE_CHILD_WHISPER_AFTER, game);
                 } else {
                     Dialogue_Start(game->dialogue, DIALOGUE_CHILD_WHISPER_FIRST, game);
                 }
                 return;
             } else if (npcs[i].label && npcs[i].label[0] == 'M') {
                 Dialogue_Start(game->dialogue, DIALOGUE_MASKED_CUSTOMER, game);
                 return;
             } else if (npcs[i].label && npcs[i].label[0] == 'C') {
                 // Reuse masked customer script as a simple passerby remark
                 Dialogue_Start(game->dialogue, DIALOGUE_MASKED_CUSTOMER, game);
                 return;
             }
         }
     }
 
     // Stalls
     for (int i = 0; i < stallCount; i++) {
         if (CheckCollisionRecs(playerRect, stalls[i].bounds)) {
             if (stalls[i].type == STALL_FOOD) {
                 if (game->anomaly1Triggered) {
                     Dialogue_Start(game->dialogue, DIALOGUE_VENDOR_AFTER_ANOMALY, game);
                 } else {
                     Dialogue_Start(game->dialogue, DIALOGUE_VENDOR_FIRST, game);
                 }
                 return;
             }
         }
     }
 }
 
 static void Game_UpdateObjective(Game *game) {
     if (game->state == GAME_STATE_TITLE) {
         game->objectiveText = "Press Enter to begin.";
         return;
     }
 
     if (!game->anomaly1Triggered) {
         game->objectiveText = "Talk to the vendor and explore the walkway.";
     } else if (!game->anomaly2Triggered) {
         game->objectiveText = "Search for a stall that doesn't remember its place.";
     } else if (!game->anomaly3Triggered) {
         game->objectiveText = "Inspect the narrow uncanny alley.";
     } else if (!game->allAnomaliesResolved) {
         game->objectiveText = "Observe any remaining distortions.";
     } else {
         game->objectiveText = "Return mentally to the entrance... the report is ready.";
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
 
     // Don't update world when paused (but still update dialogue text)
     bool allowWorldUpdate = !game->paused;
 
     UI_Update(game->ui, dt);
     Dialogue_Update(game->dialogue);
     Audio_Update(game->audio);
 
     if (allowWorldUpdate && (game->state == GAME_STATE_EXPLORE || game->state == GAME_STATE_ANOMALY_SEQUENCE)) {
         Player_Update(game->player, game->map, dt, !Dialogue_IsActive(game->dialogue));
         Anomaly_Update(game->anomalies, game, dt);
         Game_HandleInteractions(game);
 
         if (game->allAnomaliesResolved && game->state != GAME_STATE_ENDING) {
             Game_ChangeState(game, GAME_STATE_ENDING);
         }
     }
 
     if (game->state == GAME_STATE_INTRO) {
         if (!Dialogue_IsActive(game->dialogue) && Dialogue_JustClosed(game->dialogue)) {
             Game_ChangeState(game, GAME_STATE_EXPLORE);
         }
     }
 
     if (game->state == GAME_STATE_EXPLORE || game->state == GAME_STATE_ANOMALY_SEQUENCE) {
         if (game->anomaly1Triggered || game->anomaly2Triggered || game->anomaly3Triggered) {
             game->state = GAME_STATE_ANOMALY_SEQUENCE;
         }
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
 
     Map_DrawBackground(game->map);
     Player_Draw(game->player);
     Map_DrawForeground(game->map);
 
     Anomaly_DrawOverlay(game->anomalies);
 
     UI_DrawHUD(game->ui, game->screenWidth, game->screenHeight, game->objectiveText, game->paused);
     Dialogue_Draw(game->dialogue, game->screenWidth, game->screenHeight);
     UI_DrawFadeOverlay(game->ui, game->screenWidth, game->screenHeight);
 }
