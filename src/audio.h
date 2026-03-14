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
 
 // Create / destroy
 AudioManager *Audio_Create(void);
 void          Audio_Destroy(AudioManager *audio);
 
 // Per-frame
 void          Audio_Update(AudioManager *audio);
 
 // Events
 void          Audio_PlayAnomalyPing(AudioManager *audio);
 
 #endif // AUDIO_H
