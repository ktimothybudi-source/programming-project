 #include "audio.h"
 #include "raylib.h"
 #include <stdlib.h>
 
 AudioManager *Audio_Create(void) {
     AudioManager *a = (AudioManager *)malloc(sizeof(AudioManager));
     if (!a) return NULL;
 
     // Ensure audio device is initialized once in main before this is called.
 
     // Attempt to load optional assets (they may not exist)
     a->bgm = LoadMusicStream("assets/audio/night_market_bgm.ogg");
     // In Raylib, missing music usually has ctxData == NULL; guard against it
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
