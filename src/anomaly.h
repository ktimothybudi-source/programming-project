 #ifndef ANOMALY_H
 #define ANOMALY_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 typedef enum AnomalyType {
     ANOMALY_LANTERNS = 0,
     ANOMALY_STALL_SHIFT,
     ANOMALY_NPC_DIALOGUE
 } AnomalyType;
 
 typedef struct Anomaly {
     Rectangle triggerArea;
     AnomalyType type;
     bool triggered;
 } Anomaly;
 
 #define MAX_ANOMALIES 3
 
 typedef struct AnomalyManager {
     Anomaly anomalies[MAX_ANOMALIES];
     int anomalyCount;
     float lastAnomalyFlashTime;
     float flashAlpha;
 } AnomalyManager;
 
 typedef struct Game Game;
 
 // Create / destroy
 AnomalyManager *Anomaly_Create(const Game *game);
 void            Anomaly_Destroy(AnomalyManager *mgr);
 
 // Per-frame
 void            Anomaly_Update(AnomalyManager *mgr, Game *game, float dt);
 void            Anomaly_DrawOverlay(const AnomalyManager *mgr);
 
 // Utility
 bool            Anomaly_AllTriggered(const AnomalyManager *mgr);
 
 #endif // ANOMALY_H
