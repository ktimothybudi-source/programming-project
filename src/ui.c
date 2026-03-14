 #include "ui.h"
 #include "raylib.h"
 #include <stdlib.h>
 #include <math.h>
 
 UIState *UI_Create(void) {
     UIState *u = (UIState *)malloc(sizeof(UIState));
     if (!u) return NULL;
     u->fadeAlpha = 1.0f;
     u->fadeTarget = 0.0f;
     u->fadeSpeed = 1.0f;
     u->fading = true;
     u->showInstructions = true;
     return u;
 }
 
 void UI_Destroy(UIState *ui) {
     if (ui) free(ui);
 }
 
 void UI_Update(UIState *ui, float dt) {
     if (!ui) return;
 
     if (IsKeyPressed(KEY_H)) {
         ui->showInstructions = !ui->showInstructions;
     }
 
     if (ui->fading) {
         if (ui->fadeAlpha < ui->fadeTarget) {
             ui->fadeAlpha += ui->fadeSpeed * dt;
             if (ui->fadeAlpha >= ui->fadeTarget) {
                 ui->fadeAlpha = ui->fadeTarget;
                 ui->fading = false;
             }
         } else if (ui->fadeAlpha > ui->fadeTarget) {
             ui->fadeAlpha -= ui->fadeSpeed * dt;
             if (ui->fadeAlpha <= ui->fadeTarget) {
                 ui->fadeAlpha = ui->fadeTarget;
                 ui->fading = false;
             }
         }
     }
 }
 
 void UI_StartFade(UIState *ui, float targetAlpha, float speed) {
     if (!ui) return;
     ui->fadeTarget = targetAlpha;
     ui->fadeSpeed = speed;
     ui->fading = true;
 }
 
 void UI_DrawTitleScreen(const UIState *ui, int screenWidth, int screenHeight, float t) {
     (void)ui;
     ClearBackground((Color){ 5, 3, 15, 255 });
 
     const char *title = "BIT Anomalies - 夜市";
     int titleSize = 42;
     int titleWidth = MeasureText(title, titleSize);
 
     DrawText(title,
              screenWidth/2 - titleWidth/2,
              screenHeight/3 - 40,
              titleSize,
              (Color){ 240, 220, 255, 255 });
 
     const char *subtitle = "Night Market Prototype";
     int subSize = 20;
     int subWidth = MeasureText(subtitle, subSize);
     DrawText(subtitle,
              screenWidth/2 - subWidth/2,
              screenHeight/3 + 20,
              subSize,
              (Color){ 190, 170, 230, 255 });
 
     const char *hint = "[Enter] Begin Investigation";
     int hintSize = 18;
     int hintWidth = MeasureText(hint, hintSize);
     unsigned char alpha = (unsigned char)(130 + 50 * (0.5f + 0.5f * sinf(t * 2.8f)));
     DrawText(hint,
              screenWidth/2 - hintWidth/2,
              screenHeight*2/3,
              hintSize,
              (Color){ 220, 220, 240, alpha });
 
     const char *controls = "WASD/Arrows: Move   E: Interact   Space/Enter: Advance text\nEsc: Pause   R: Restart   H: Toggle help overlay";
     int cSize = 14;
     int cWidth = MeasureText(controls, cSize);
     DrawText(controls,
              screenWidth/2 - cWidth/2,
              screenHeight - 80,
              cSize,
              (Color){ 180, 180, 210, 255 });
 }
 
 void UI_DrawHUD(const UIState *ui, int screenWidth, int screenHeight, const char *objective, bool paused) {
     (void)screenHeight;
 
     // Objective
     if (objective) {
         DrawRectangle(16, 16, screenWidth/2, 48, (Color){ 8, 8, 20, 200 });
         DrawRectangleLines(16, 16, screenWidth/2, 48, (Color){ 140, 140, 200, 255 });
         DrawText("Objective:", 24, 22, 16, (Color){ 230, 230, 255, 255 });
         DrawText(objective, 24, 42, 14, (Color){ 210, 210, 230, 255 });
     }
 
     if (ui->showInstructions) {
         const char *controls = "WASD/Arrows: Move   E: Interact   Esc: Pause   R: Restart   H: Hide help";
         int size = 14;
         DrawRectangle(16, screenHeight - 48, screenWidth - 32, 36, (Color){ 8, 8, 18, 200 });
         DrawText(controls, 24, screenHeight - 40, size, (Color){ 190, 190, 220, 255 });
     }
 
     if (paused) {
         const char *text = "Paused";
         int size = 30;
         int width = MeasureText(text, size);
         DrawText(text,
                  screenWidth/2 - width/2,
                  screenHeight/2 - 20,
                  size,
                  (Color){ 255, 255, 255, 230 });
     }
 }
 
 void UI_DrawFadeOverlay(const UIState *ui, int screenWidth, int screenHeight) {
     if (!ui) return;
     if (ui->fadeAlpha <= 0.0f) return;
     unsigned char alpha = (unsigned char)(ui->fadeAlpha * 255.0f);
     DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, alpha });
 }
