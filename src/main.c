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
 *
 * Window size:
 *   • Default: 1024×768 (4:3).
 *   • Optional file assets/window_size.txt — first line: two integers, width height.
 *   • Optional args: night_market [width] [height]  (overrides the file)
 *   • -h or --help prints usage.
 * ============================================================================
 */

#include "raylib.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SCREEN_WIDTH   1024
#define DEFAULT_SCREEN_HEIGHT  768
#define MIN_SCREEN_WIDTH       640
#define MIN_SCREEN_HEIGHT      480
#define MAX_SCREEN_WIDTH       3840
#define MAX_SCREEN_HEIGHT      2160

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool parse_positive_int(const char *s, int *out) {
    if (!s || !*s) return false;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 1L || v > 100000L)
        return false;
    *out = (int)v;
    return true;
}

static bool load_window_size_file(const char *path, int *w, int *h) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    int a = 0, b = 0;
    int n = fscanf(f, "%d %d", &a, &b);
    fclose(f);
    if (n != 2 || a < 1 || b < 1) return false;
    *w = a;
    *h = b;
    return true;
}

static void resolve_window_size(int argc, char **argv, int *outW, int *outH) {
    int w = DEFAULT_SCREEN_WIDTH;
    int h = DEFAULT_SCREEN_HEIGHT;

    const char *cfg = "assets/window_size.txt";
    if (load_window_size_file(cfg, &w, &h)) {
        /* file wins over defaults */
    }

    if (argc >= 3) {
        int aw = 0, ah = 0;
        if (parse_positive_int(argv[1], &aw) && parse_positive_int(argv[2], &ah)) {
            w = aw;
            h = ah;
        } else {
            fprintf(stderr,
                    "Invalid width/height. Use integers, e.g. %s 1024 768\n",
                    argc > 0 ? argv[0] : "night_market");
            exit(1);
        }
    } else if (argc == 2) {
        fprintf(stderr, "Need both width and height, or use -h for help.\n");
        exit(1);
    }

    w = clampi(w, MIN_SCREEN_WIDTH, MAX_SCREEN_WIDTH);
    h = clampi(h, MIN_SCREEN_HEIGHT, MAX_SCREEN_HEIGHT);
    *outW = w;
    *outH = h;
}

int main(int argc, char **argv) {
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        printf("BIT Anomalies — window size\n"
               "  Default: %d×%d\n"
               "  Config:  assets/window_size.txt (first line: width height)\n"
               "  Args:    %s [width] [height]\n"
               "  Allowed: %d…%d × %d…%d\n",
               DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT,
               argv[0],
               MIN_SCREEN_WIDTH, MAX_SCREEN_WIDTH,
               MIN_SCREEN_HEIGHT, MAX_SCREEN_HEIGHT);
        return 0;
    }

    int screenWidth = DEFAULT_SCREEN_WIDTH;
    int screenHeight = DEFAULT_SCREEN_HEIGHT;
    resolve_window_size(argc, argv, &screenWidth, &screenHeight);

    /*
     * InitWindow asks the operating system to create a window.
     * The last argument is the text shown in the window's title bar.
     */
    InitWindow(screenWidth, screenHeight, "The Night Market");

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
