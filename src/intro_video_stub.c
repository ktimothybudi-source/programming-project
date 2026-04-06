#include "intro_video.h"
#include <stdlib.h>

IntroVideoPlayer *IntroVideo_Open(const char *pathUtf8, int screenW, int screenH) {
    (void)pathUtf8;
    (void)screenW;
    (void)screenH;
    return NULL;
}

void IntroVideo_Close(IntroVideoPlayer *p) {
    (void)p;
}

bool IntroVideo_Update(IntroVideoPlayer *p, float dt) {
    (void)p;
    (void)dt;
    return false;
}

Texture2D IntroVideo_GetTexture(const IntroVideoPlayer *p) {
    (void)p;
    Texture2D z = { 0 };
    return z;
}
