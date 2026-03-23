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

AudioManager *Audio_Create(void) {
    AudioManager *a = (AudioManager *)malloc(sizeof(AudioManager));
    if (!a) return NULL;

    /* main.c must call InitAudioDevice() before this runs. */

    a->bgm = LoadMusicStream("assets/audio/night_market_bgm.ogg");
    a->hasBgm = (a->bgm.ctxData != NULL);

    a->anomalyPing = LoadSound("assets/audio/anomaly_ping.wav");
    a->hasAnomalyPing = (a->anomalyPing.frameCount > 0);

    if (a->hasBgm) {
        a->bgm.looping = true;
        PlayMusicStream(a->bgm);
        SetMusicVolume(a->bgm, 0.5f);
    }

    return a;
}

void Audio_Destroy(AudioManager *audio) {
    if (!audio) return;
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
