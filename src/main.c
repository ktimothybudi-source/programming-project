 #include "raylib.h"
 #include "game.h"
 
 int main(void) {
     const int screenWidth = 800;
     const int screenHeight = 600;
 
     InitWindow(screenWidth, screenHeight, "BIT Anomalies - Night Market");
     SetTargetFPS(60);
 
     // Audio is optional, but we still initialize the device.
     InitAudioDevice();
 
     Game *game = Game_Create(screenWidth, screenHeight);
     if (!game) {
         CloseAudioDevice();
         CloseWindow();
         return 1;
     }
 
     while (!WindowShouldClose()) {
         float dt = GetFrameTime();
         if (dt > 0.033f) dt = 0.033f; // clamp to avoid huge steps
 
         Game_Update(game, dt);
 
         BeginDrawing();
         Game_Draw(game);
         EndDrawing();
     }
 
     Game_Destroy(game);
     CloseAudioDevice();
     CloseWindow();
 
     return 0;
 }
