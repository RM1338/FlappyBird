#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include "player.h"   // brings in Bird

// --- CONFIGURATION CONSTANTS ---
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define PIPE_SPEED      150.0f
#define PIPE_WIDTH      80
#define PIPE_SPAWN_TIME 2.0f
#define MIN_GAP_SIZE    100
#define MAX_GAP_SIZE    160

// --- ENUMS ---
typedef enum GameState {
    GAME_WAITING,
    GAME_RUNNING,
    GAME_OVER
} GameState;

// --- STRUCTURES ---
typedef struct Pipe {
    Rectangle top;
    Rectangle bottom;
    bool active;
    bool scored;
} Pipe;

typedef struct Game {
    GameState state;
    int score;
    int highScore;

    Bird bird;

    Pipe pipes[16];
    float pipeSpawnTimer;
    int pipeCount;

    Texture2D texBird;
    Texture2D texPipe;
    Texture2D texBg;

    Sound sFlap;
    Sound sScore;
    Sound sHit;

    Font font;
} Game;

// --- FUNCTION PROTOTYPES ---
void InitGame(Game *game);
void UpdateGame(Game *game, float dt);
void DrawGame(const Game *game);
void UnloadGame(Game *game);
void ResetGame(Game *game);
void SpawnPipe(Game *game);

#endif // GAME_H