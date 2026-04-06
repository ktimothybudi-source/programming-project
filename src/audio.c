/*
 * ============================================================================
 * audio.c — LOAD MUSIC, PLAY SOUNDS, STREAM MUSIC EACH FRAME
 * ============================================================================
 *
 * Raylib separates:
 *   Music  — streamed from disk (good for long BGM), needs UpdateMusicStream every frame
 *   Sound  — short clip loaded fully into memory
 *
 * LoadMusicStream / LoadSound return empty objects if the path is wrong; we check
 * ctxData / frameCount before using them.
 * ============================================================================
 */

#include "audio.h"
#include "raylib.h"
#include <stdlib.h>

typedef struct {
    AudioSfxId id;
    const char *path;
} SfxEntry;

static const SfxEntry SFX_TABLE[] = {
    { AUDIO_SFX_SWIPE,       "assets/audio/sfx/swipe.mp3" },
    { AUDIO_SFX_MOP_DRAG,    "assets/audio/sfx/mop_drag.mp3" },
    { AUDIO_SFX_RADIO_ON,    "assets/audio/sfx/radio_on.mp3" },
    { AUDIO_SFX_RADIO_TUNE,  "assets/audio/sfx/radio_tune.mp3" },
    { AUDIO_SFX_KNIFE,       "assets/audio/sfx/knife.mp3" },
    { AUDIO_SFX_LOCKER,      "assets/audio/sfx/locker.mp3" },
    { AUDIO_SFX_TRASH,       "assets/audio/sfx/trash.mp3" },
    { AUDIO_SFX_DISHES,      "assets/audio/sfx/dishes.mp3" },
    { AUDIO_SFX_FOOTSTEPS,   "assets/audio/sfx/footsteps.mp3" },
};

AudioManager *Audio_Create(void) {
    AudioManager *a = (AudioManager *)malloc(sizeof(AudioManager));
    if (!a) return NULL;

    /* main.c must call InitAudioDevice() before this runs. */

    a->bgm = LoadMusicStream("assets/audio/night_market_bgm.ogg");
    a->hasBgm = (a->bgm.ctxData != NULL);

    a->anomalyPing = LoadSound("assets/audio/anomaly_ping.wav");
    a->hasAnomalyPing = (a->anomalyPing.frameCount > 0);

    for (int i = 0; i < AUDIO_SFX_COUNT; i++) {
        a->sfx[i] = (Sound){ 0 };
        a->hasSfx[i] = false;
    }
    for (size_t i = 0; i < sizeof(SFX_TABLE) / sizeof(SFX_TABLE[0]); i++) {
        int idx = (int)SFX_TABLE[i].id;
        if (idx < 0 || idx >= AUDIO_SFX_COUNT) continue;
        Sound s = LoadSound(SFX_TABLE[i].path);
        a->sfx[idx] = s;
        a->hasSfx[idx] = (s.frameCount > 0);
        if (a->hasSfx[idx])
            SetSoundVolume(s, 0.72f);
    }

    if (a->hasBgm) {
        a->bgm.looping = true;
        PlayMusicStream(a->bgm);
        SetMusicVolume(a->bgm, 0.5f);
    }

    return a;
}

void Audio_Destroy(AudioManager *audio) {
    if (!audio) return;
    for (int i = 0; i < AUDIO_SFX_COUNT; i++) {
        if (audio->hasSfx[i])
            UnloadSound(audio->sfx[i]);
    }
    if (audio->hasAnomalyPing) UnloadSound(audio->anomalyPing);
    if (audio->hasBgm) UnloadMusicStream(audio->bgm);
    free(audio);
}

void Audio_Update(AudioManager *audio) {
    if (!audio) return;
    if (audio->hasBgm) {
        UpdateMusicStream(audio->bgm);
    }
}

void Audio_PlayAnomalyPing(AudioManager *audio) {
    if (!audio) return;
    if (audio->hasAnomalyPing) {
        PlaySound(audio->anomalyPing);
    }
}

void Audio_PlaySfx(AudioManager *audio, AudioSfxId id) {
    if (!audio || id >= AUDIO_SFX_COUNT) return;
    if (audio->hasSfx[id])
        PlaySound(audio->sfx[id]);
}

void Audio_StopAllSfx(AudioManager *audio) {
    if (!audio) return;
    for (int i = 0; i < AUDIO_SFX_COUNT; i++) {
        if (audio->hasSfx[i])
            StopSound(audio->sfx[i]);
    }
}
