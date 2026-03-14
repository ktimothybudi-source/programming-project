 #ifndef GAME_H
 #define GAME_H
 
 #include "raylib.h"
 #include <stdbool.h>
 
 // Forward declarations of modules
 typedef struct Player Player;
 typedef struct Map Map;
 typedef struct DialogueSystem DialogueSystem;
 typedef struct AnomalyManager AnomalyManager;
 typedef struct AudioManager AudioManager;
 typedef struct UIState UIState;
 
 // Core game states
 typedef enum GameState {
     GAME_STATE_TITLE = 0,
     GAME_STATE_INTRO,
     GAME_STATE_EXPLORE,
     GAME_STATE_ANOMALY_SEQUENCE,
     GAME_STATE_ENDING
 } GameState;
 
 // High-level game container
 typedef struct Game {
     GameState state;
     float stateTime;
     bool paused;
     bool requestRestart;
 
     Map *map;
     Player *player;
     DialogueSystem *dialogue;
     AnomalyManager *anomalies;
     AudioManager *audio;
     UIState *ui;
 
     // Simple progression flags
     bool anomaly1Triggered;
     bool anomaly2Triggered;
     bool anomaly3Triggered;
     bool allAnomaliesResolved;
 
     // Current objective text
     const char *objectiveText;
 
     // Cached window size
     int screenWidth;
     int screenHeight;
 } Game;
 
 // Lifecycle
 Game *Game_Create(int screenWidth, int screenHeight);
 void   Game_Destroy(Game *game);
 
 // Update & draw
 void   Game_Update(Game *game, float dt);
 void   Game_Draw(Game *game);
 
 // Utility
 void   Game_Restart(Game *game);
 
 #endif // GAME_H
