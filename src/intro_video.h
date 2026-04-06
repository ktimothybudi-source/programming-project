/*
 * Opening cinematic: plays assets/video/intro.mp4 before the title screen.
 * macOS: AVFoundation decoder. Other platforms: stub (skips to title if no impl).
 */
#ifndef INTRO_VIDEO_H
#define INTRO_VIDEO_H

#include "raylib.h"
#include <stdbool.h>

typedef struct IntroVideoPlayer IntroVideoPlayer;

IntroVideoPlayer *IntroVideo_Open(const char *pathUtf8, int screenW, int screenH);
void              IntroVideo_Close(IntroVideoPlayer *p);
/* Advance playback; returns false when finished (or error). */
bool              IntroVideo_Update(IntroVideoPlayer *p, float dt);
Texture2D         IntroVideo_GetTexture(const IntroVideoPlayer *p);

#endif
