/*
 * ============================================================================
 * audio.h — BACKGROUND MUSIC AND SOUND EFFECTS (OPTIONAL)
 * ============================================================================
 *
 * If files under assets/audio/ are missing, hasBgm / hasAnomalyPing / hasSfx[]
 * stay false and we skip playing — the game still runs without sound files.
 * ============================================================================
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include <stdbool.h>

typedef enum {
    AUDIO_SFX_SWIPE = 0,
    AUDIO_SFX_MOP_DRAG,
    AUDIO_SFX_RADIO_ON,
    AUDIO_SFX_RADIO_TUNE,
    AUDIO_SFX_KNIFE,
    AUDIO_SFX_LOCKER,
    AUDIO_SFX_TRASH,
    AUDIO_SFX_DISHES,
    AUDIO_SFX_FOOTSTEPS,
    AUDIO_SFX_COUNT
} AudioSfxId;

typedef struct AudioManager {
    Music bgm;
    bool hasBgm;

    Sound anomalyPing;
    bool hasAnomalyPing;

    Sound sfx[AUDIO_SFX_COUNT];
    bool hasSfx[AUDIO_SFX_COUNT];
} AudioManager;

AudioManager *Audio_Create(void);
void          Audio_Destroy(AudioManager *audio);

void          Audio_Update(AudioManager *audio);

void          Audio_PlayAnomalyPing(AudioManager *audio);
void          Audio_PlaySfx(AudioManager *audio, AudioSfxId id);
/* Stops every loaded SFX instance (e.g. when a minigame ends so long clips don’t linger). */
void          Audio_StopAllSfx(AudioManager *audio);

#endif /* AUDIO_H */
