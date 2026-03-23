/*
 * ============================================================================
 * anomaly.h — SCREEN FLASH / "UNCANNY" VISUAL LAYER
 * ============================================================================
 *
 * An "anomaly" in the design doc could be many things. In this prototype the
 * AnomalyManager mostly drives a FULL-SCREEN color overlay that fades (flash).
 * It does NOT contain the main story logic; game.c decides when lights go out.
 *
 * AnomalyType / struct Anomaly are reserved if you later want trigger areas
 * in the map; currently the update path uses Game flags more than those.
 * ============================================================================
 */

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

AnomalyManager *Anomaly_Create(const Game *game);
void            Anomaly_Destroy(AnomalyManager *mgr);

void            Anomaly_Update(AnomalyManager *mgr, Game *game, float dt);
void            Anomaly_DrawOverlay(const AnomalyManager *mgr);

bool            Anomaly_AllTriggered(const AnomalyManager *mgr);

#endif /* ANOMALY_H */
