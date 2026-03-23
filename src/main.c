/*
 * ============================================================================
 * main.c — WHERE THE PROGRAM STARTS
 * ============================================================================
 *
 * Every C program has a function called "main". The computer runs main() first.
 * This file does NOT contain the game rules; it only:
 *   1) Opens a WINDOW on your screen (the game box)
 *   2) Creates the GAME object (everything else lives inside game.c)
 *   3) Repeats forever until you close the window: UPDATE then DRAW
 *
 * Think of it like a flipbook: each "frame" is one picture. Sixty times per
 * second we update logic (move player, check keys) and draw the new picture.
 *
 * "dt" = delta time = how many seconds passed since the last frame. We use it
 * so movement looks smooth even if the frame rate changes a little.
 * ============================================================================
 */

#include "raylib.h"
#include "game.h"

int main(void) {
    /* How big the window is in pixels (width x height). */
    const int screenWidth = 800;
    const int screenHeight = 600;

    /*
     * InitWindow asks the operating system to create a window.
     * The last argument is the text shown in the window's title bar.
     */
    InitWindow(screenWidth, screenHeight, "BIT Anomalies - 夜市 (Bohou Supermarket)");

    /*
     * FPS = frames per second. 60 means we try to redraw 60 times per second.
     */
    SetTargetFPS(60);

    /*
     * Sound needs to be turned on before we load any music or sound effects.
     * If this fails on some computers, the rest of the game can still run.
     */
    InitAudioDevice();

    /*
     * Game_Create allocates memory and builds the map, player, dialogue, etc.
     * It returns a POINTER — think of it as the address of one big struct Game.
     * If malloc fails, we get NULL and we must exit without crashing.
     */
    Game *game = Game_Create(screenWidth, screenHeight);
    if (!game) {
        CloseAudioDevice();
        CloseWindow();
        return 1;  /* Non-zero return value usually means "error" to the OS. */
    }

    /*
     * MAIN LOOP — this runs until the user clicks the X or presses Alt+F4.
     * WindowShouldClose() becomes true when the user wants to quit.
     */
    while (!WindowShouldClose()) {
        /*
         * GetFrameTime() returns seconds since the last frame (a small number
         * like 0.016). We clamp it so if the game freezes, we don't jump too far.
         */
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        /*
         * UPDATE: move things, read keyboard, advance dialogue — no drawing yet.
         */
        Game_Update(game, dt);

        /*
         * DRAW: raylib requires BeginDrawing before any DrawXXX calls, and
         * EndDrawing when this frame's picture is complete.
         */
        BeginDrawing();
        Game_Draw(game);
        EndDrawing();
    }

    /*
     * Cleanup: free memory and close window in reverse order of creation.
     */
    Game_Destroy(game);
    CloseAudioDevice();
    CloseWindow();

    return 0;  /* Zero means "program finished successfully". */
}
