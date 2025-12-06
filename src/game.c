#include "game.h"
#include "player.h"
#include <stdlib.h>
#include <time.h>

#define PIPE_CAP_HEIGHT 24   // pixels from top of texture used as cap

// Assets
static void InitAssets(Game *game) {
    game->texBird = LoadTexture("assets/bird.png");
    game->texPipe = LoadTexture("assets/pipe.png");   // 80 x 217, cap at top
    game->texBg   = LoadTexture("assets/bg.png");

    InitAudioDevice();
    game->sFlap  = LoadSound("assets/sounds/flap.wav");
    game->sScore = LoadSound("assets/sounds/score.wav");
    game->sHit   = LoadSound("assets/sounds/hit.wav");
}

static void UnloadAssets(Game *game) {
    UnloadTexture(game->texBird);
    UnloadTexture(game->texPipe);
    UnloadTexture(game->texBg);

    UnloadSound(game->sFlap);
    UnloadSound(game->sScore);
    UnloadSound(game->sHit);
    CloseAudioDevice();
}

// Game lifecycle
void InitGame(Game *game) {
    srand((unsigned int)time(NULL));

    game->state = GAME_RUNNING;
    game->score = 0;
    game->highScore = 0;
    game->pipeSpawnTimer = 0.0f;
    game->pipeCount = 0;

    InitAssets(game);
    InitBird(&game->bird);

    for (int i = 0; i < 16; i++) {
        game->pipes[i].active = false;
        game->pipes[i].scored = false;
    }

    SpawnPipe(game);
}

void ResetGame(Game *game) {
    game->state = GAME_RUNNING;
    game->score = 0;
    game->pipeSpawnTimer = 0.0f;
    game->pipeCount = 0;

    InitBird(&game->bird);
    for (int i = 0; i < 16; i++) {
        game->pipes[i].active = false;
        game->pipes[i].scored = false;
    }
    SpawnPipe(game);
}

void UnloadGame(Game *game) {
    UnloadAssets(game);
}

// Pipes
void SpawnPipe(Game *game) {
    if (game->pipeCount >= 16) game->pipeCount = 0;

    Pipe *pipe = &game->pipes[game->pipeCount];

    int gapSize = MIN_GAP_SIZE + rand() % (MAX_GAP_SIZE - MIN_GAP_SIZE + 1);
    int minY = 60;
    int maxY = SCREEN_HEIGHT - 60 - gapSize;
    int gapY = minY + rand() % (maxY - minY + 1);

    pipe->top = (Rectangle){ (float)SCREEN_WIDTH, 0.0f, PIPE_WIDTH, (float)gapY };
    pipe->bottom = (Rectangle){
        (float)SCREEN_WIDTH,
        (float)(gapY + gapSize),
        PIPE_WIDTH,
        (float)(SCREEN_HEIGHT - (gapY + gapSize))
    };

    pipe->active = true;
    pipe->scored = false;
    game->pipeCount++;
}

// Update
void UpdateGame(Game *game, float dt) {
    if (game->state != GAME_RUNNING) return;

    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        BirdFlap(&game->bird);
        PlaySound(game->sFlap);
    }

    UpdateBird(&game->bird, dt);

    if (BirdHitWorld(&game->bird)) {
        game->state = GAME_OVER;
        PlaySound(game->sHit);
        return;
    }

    Rectangle birdRect = BirdGetRect(&game->bird);

    for (int i = 0; i < game->pipeCount; i++) {
        Pipe *p = &game->pipes[i];
        if (!p->active) continue;

        p->top.x    -= PIPE_SPEED * dt;
        p->bottom.x -= PIPE_SPEED * dt;

        if (p->top.x + PIPE_WIDTH < 0.0f) {
            p->active = false;
        }

        if (CheckCollisionRecs(birdRect, p->top) ||
            CheckCollisionRecs(birdRect, p->bottom)) {
            game->state = GAME_OVER;
            PlaySound(game->sHit);
            return;
        }

        float pipeCenterX = p->top.x + PIPE_WIDTH * 0.5f;
        if (!p->scored && birdRect.x > pipeCenterX) {
            p->scored = true;
            game->score++;
            if (game->score > game->highScore) {
                game->highScore = game->score;
            }
            PlaySound(game->sScore);
        }
    }

    game->pipeSpawnTimer += dt;
    if (game->pipeSpawnTimer >= PIPE_SPAWN_TIME) {
        game->pipeSpawnTimer = 0.0f;
        SpawnPipe(game);
    }
}

// Draw
void DrawGame(const Game *game) {
    // Background
    DrawTexture(game->texBg, 0, 0, WHITE);

    // Texture regions for pipe.png (80 x 217, cap at top)
    Rectangle srcCap  = { 0, 0, PIPE_WIDTH, PIPE_CAP_HEIGHT };
    Rectangle srcBody = {
        0,
        PIPE_CAP_HEIGHT,
        PIPE_WIDTH,
        game->texPipe.height - PIPE_CAP_HEIGHT
    };

    // Pipes
    for (int i = 0; i < game->pipeCount; i++) {
        const Pipe *p = &game->pipes[i];
        if (!p->active) continue;

        // ---------- TOP PIPE ----------
        float topBodyHeight = p->top.height - PIPE_CAP_HEIGHT;
        if (topBodyHeight < 0) topBodyHeight = 0;

        Rectangle dstTopBody = {
            p->top.x,
            p->top.y,
            PIPE_WIDTH,
            topBodyHeight
        };
        DrawTexturePro(game->texPipe, srcBody, dstTopBody,
                       (Vector2){0,0}, 0.0f, WHITE);

        Rectangle dstTopCap = {
            p->top.x,
            p->top.y + topBodyHeight,
            PIPE_WIDTH,
            PIPE_CAP_HEIGHT
        };
        DrawTexturePro(game->texPipe, srcCap, dstTopCap,
                       (Vector2){0,0}, 0.0f, WHITE);

        // ---------- BOTTOM PIPE ----------
        float bottomBodyHeight = p->bottom.height - PIPE_CAP_HEIGHT;
        if (bottomBodyHeight < 0) bottomBodyHeight = 0;

        Rectangle dstBottomBody = {
            p->bottom.x,
            p->bottom.y + PIPE_CAP_HEIGHT,
            PIPE_WIDTH,
            bottomBodyHeight
        };
        DrawTexturePro(game->texPipe, srcBody, dstBottomBody,
                       (Vector2){0,0}, 0.0f, WHITE);

        Rectangle dstBottomCap = {
            p->bottom.x,
            p->bottom.y,
            PIPE_WIDTH,
            PIPE_CAP_HEIGHT
        };
        DrawTexturePro(game->texPipe, srcCap, dstBottomCap,
                       (Vector2){0,0}, 0.0f, WHITE);
    }

    // Bird
    DrawBirdSprite(game);

    // Score text
    DrawText(TextFormat("SCORE: %d", game->score), 20, 20, 32, BLACK);
    DrawText(TextFormat("BEST: %d", game->highScore), 20, 60, 24, DARKGRAY);

    // Game over overlay
    if (game->state == GAME_OVER) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
        const char *msg = "GAME OVER";
        int msgW = MeasureText(msg, 48);
        DrawText(msg, SCREEN_WIDTH/2 - msgW/2, SCREEN_HEIGHT/2 - 40, 48, RAYWHITE);

        const char *hint = "Press SPACE or Left Click to restart";
        int hintW = MeasureText(hint, 20);
        DrawText(hint, SCREEN_WIDTH/2 - hintW/2, SCREEN_HEIGHT/2 + 20, 20, RAYWHITE);
    }
}
