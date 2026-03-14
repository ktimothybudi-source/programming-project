#include "anomaly.h"
#include "game.h"
#include "raylib.h"
#include <stdlib.h>

AnomalyManager *Anomaly_Create(const Game *game) {
    (void)game;
    AnomalyManager *a = (AnomalyManager *)malloc(sizeof(AnomalyManager));
    if (!a) return NULL;
    a->anomalyCount = 0;
    a->lastAnomalyFlashTime = 0.0f;
    a->flashAlpha = 0.0f;
    return a;
}

void Anomaly_Destroy(AnomalyManager *mgr) {
    if (mgr) free(mgr);
}

bool Anomaly_AllTriggered(const AnomalyManager *mgr) {
    (void)mgr;
    return true;
}

void Anomaly_Update(AnomalyManager *mgr, Game *game, float dt) {
    if (!mgr || !game) return;

    if (game->lightsOff) {
        mgr->flashAlpha = 0.0f;
    }
    if (game->ending1Triggered) {
        mgr->flashAlpha = 0.3f;
    }

    if (mgr->flashAlpha > 0.0f) {
        mgr->lastAnomalyFlashTime += dt;
        mgr->flashAlpha -= dt * 0.5f;
        if (mgr->flashAlpha < 0.0f) mgr->flashAlpha = 0.0f;
    }
}

void Anomaly_DrawOverlay(const AnomalyManager *mgr) {
    if (!mgr) return;
    if (mgr->flashAlpha <= 0.0f) return;

    Color c = { 230, 220, 255, (unsigned char)(mgr->flashAlpha * 180.0f) };
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), c);
}
