/*
 * ============================================================================
 * audio.h — BACKGROUND MUSIC AND SOUND EFFECTS (OPTIONAL)
 * ============================================================================
 *
 * If the files under assets/audio/ are missing, we set hasBgm / hasAnomalyPing
 * to false and skip playing — the game still runs without sound files.
 * ============================================================================
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include <stdbool.h>

typedef struct AudioManager {
    Music bgm;
    bool hasBgm;

    Sound anomalyPing;
    bool hasAnomalyPing;
} AudioManager;

AudioManager *Audio_Create(void);
void          Audio_Destroy(AudioManager *audio);

void          Audio_Update(AudioManager *audio);

void          Audio_PlayAnomalyPing(AudioManager *audio);

#endif /* AUDIO_H */
