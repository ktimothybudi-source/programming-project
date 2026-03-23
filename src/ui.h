/*
 * ============================================================================
 * ui.h — TITLE SCREEN, HUD (OBJECTIVE TEXT), PAUSE, FADE
 * ============================================================================
 *
 * UIState holds:
 *   fadeAlpha / fadeTarget / fadeSpeed / fading — for black fade in/out
 *   showInstructions — whether to draw the help bar at the bottom (H toggles)
 * ============================================================================
 */

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

UIState *UI_Create(void);
void     UI_Destroy(UIState *ui);

void     UI_Update(UIState *ui, float dt);

void     UI_StartFade(UIState *ui, float targetAlpha, float speed);

void     UI_DrawTitleScreen(const UIState *ui, int screenWidth, int screenHeight, float t);
void     UI_DrawHUD(const UIState *ui, int screenWidth, int screenHeight, const char *objective, bool paused);
void     UI_DrawFadeOverlay(const UIState *ui, int screenWidth, int screenHeight);

#endif /* UI_H */
