 #include "anomaly.h"
 #include "game.h"
 #include "map.h"
 #include "player.h"
 #include "audio.h"
 #include "dialogue.h"
 #include "raylib.h"
 #include <stdlib.h>
 
 static void SetupAnomalies(AnomalyManager *mgr, const Game *game);
 
 AnomalyManager *Anomaly_Create(const Game *game) {
     AnomalyManager *a = (AnomalyManager *)malloc(sizeof(AnomalyManager));
     if (!a) return NULL;
     a->anomalyCount = 0;
     a->lastAnomalyFlashTime = 0.0f;
     a->flashAlpha = 0.0f;
     SetupAnomalies(a, game);
     return a;
 }
 
 void Anomaly_Destroy(AnomalyManager *mgr) {
     if (mgr) free(mgr);
 }
 
 static void SetupAnomalies(AnomalyManager *mgr, const Game *game) {
     (void)game;
     mgr->anomalyCount = 0;
 
     // 1: Lantern color anomaly (near central walkway middle)
     mgr->anomalies[mgr->anomalyCount++] = (Anomaly){
         (Rectangle){ 360.0f, 260.0f, 120.0f, 120.0f },
         ANOMALY_LANTERNS,
         false
     };
 
     // 2: Stall appears shifted (near misc stall)
     mgr->anomalies[mgr->anomalyCount++] = (Anomaly){
         (Rectangle){ 260.0f, 420.0f, 160.0f, 80.0f },
         ANOMALY_STALL_SHIFT,
         false
     };
 
     // 3: NPC dialogue rewrite (uncanny alley)
     mgr->anomalies[mgr->anomalyCount++] = (Anomaly){
         (Rectangle){ 580.0f, 260.0f, 140.0f, 140.0f },
         ANOMALY_NPC_DIALOGUE,
         false
     };
 }
 
 bool Anomaly_AllTriggered(const AnomalyManager *mgr) {
     for (int i = 0; i < mgr->anomalyCount; i++) {
         if (!mgr->anomalies[i].triggered) return false;
     }
     return true;
 }
 
 void Anomaly_Update(AnomalyManager *mgr, Game *game, float dt) {
     if (!mgr || !game || !game->player || !game->map) return;
 
     Rectangle playerRect = Player_GetBounds(game->player);
 
     for (int i = 0; i < mgr->anomalyCount; i++) {
         Anomaly *a = &mgr->anomalies[i];
         if (a->triggered) continue;
 
         if (CheckCollisionRecs(playerRect, a->triggerArea)) {
             a->triggered = true;
             mgr->flashAlpha = 0.7f;
             mgr->lastAnomalyFlashTime = 0.0f;
             Audio_PlayAnomalyPing(game->audio);
 
             switch (a->type) {
                 case ANOMALY_LANTERNS:
                     game->map->lanternsStrangeColors = true;
                     game->anomaly1Triggered = true;
                     // Play fake announcement dialogue
                     Dialogue_Start(game->dialogue, DIALOGUE_FAKE_ANNOUNCEMENT, game);
                     break;
                 case ANOMALY_STALL_SHIFT:
                     game->map->stallShifted = true;
                     // Move the "Lost & Found" stall slightly
                     for (int s = 0; s < game->map->stallCount; s++) {
                         if (game->map->stalls[s].type == STALL_MISC) {
                             game->map->stalls[s].bounds.x += 90.0f;
                             game->map->stalls[s].bounds.y -= 20.0f;
                         }
                     }
                     game->anomaly2Triggered = true;
                     break;
                 case ANOMALY_NPC_DIALOGUE:
                     game->anomaly3Triggered = true;
                     break;
                 default: break;
             }
         }
     }
 
     // Flash overlay decay
     if (mgr->flashAlpha > 0.0f) {
         mgr->lastAnomalyFlashTime += dt;
         mgr->flashAlpha -= dt * 0.7f;
         if (mgr->flashAlpha < 0.0f) mgr->flashAlpha = 0.0f;
     }
 
     if (!game->allAnomaliesResolved && Anomaly_AllTriggered(mgr)) {
         game->allAnomaliesResolved = true;
     }
 }
 
 void Anomaly_DrawOverlay(const AnomalyManager *mgr) {
     if (!mgr) return;
     if (mgr->flashAlpha <= 0.0f) return;
 
     Color c = { 230, 220, 255, (unsigned char)(mgr->flashAlpha * 180.0f) };
     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), c);
 }
