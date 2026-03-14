 #ifndef UI_H
 #define UI_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 typedef struct UIState {
     float fadeAlpha;
     float fadeTarget;
     float fadeSpeed;
     bool fading;
 
     bool showInstructions;
 } UIState;
 
 // Create / destroy
 UIState *UI_Create(void);
 void     UI_Destroy(UIState *ui);
 
 // Per-frame
 void     UI_Update(UIState *ui, float dt);
 
 // Controls
 void     UI_StartFade(UIState *ui, float targetAlpha, float speed);
 
 // Draw
 void     UI_DrawTitleScreen(const UIState *ui, int screenWidth, int screenHeight, float t);
 void     UI_DrawHUD(const UIState *ui, int screenWidth, int screenHeight, const char *objective, bool paused);
 void     UI_DrawFadeOverlay(const UIState *ui, int screenWidth, int screenHeight);
 
 #endif // UI_H
