#ifndef GAME_H
#define GAME_H

#include "raylib.h"

// Window
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Bird physics
#define GRAVITY        420.0f
#define FLAP_STRENGTH -260.0f

// Pipes
#define PIPE_WIDTH 80
#define PIPE_HEIGHT 400
#define PIPE_SPEED 160.0f
#define PIPE_SPAWN_TIME 2.0f

// Random gap size between top and bottom pipes
#define MIN_GAP_SIZE 135
#define MAX_GAP_SIZE 185

typedef enum {
    GAME_WAITING,   // before first tap after launch
    GAME_RUNNING,   // active play
    GAME_OVER       // death screen
} GameState;

typedef struct Bird {
    Vector2 position;
    Vector2 size;
    float velocity;
    float acceleration;
    int frame;
    float frameTimer;
} Bird;

typedef struct {
    Rectangle top;
    Rectangle bottom;
    bool active;
    bool scored;
} Pipe;

typedef struct {
    GameState state;
    int score;
    int highScore;
    float pipeSpawnTimer;
    Pipe pipes[16];
    int pipeCount;

    Texture2D texBird;
    Texture2D texPipe;
    Texture2D texBg;

    Sound sFlap;
    Sound sScore;
    Sound sHit;

    Font font;     // pixel font used for UI

    Bird bird;
} Game;

void InitGame(Game *game);
void UpdateGame(Game *game, float dt);
void DrawGame(const Game *game);
void ResetGame(Game *game);
void UnloadGame(Game *game);
void SpawnPipe(Game *game);

#endif