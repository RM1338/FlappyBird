// src/game.c
#include "game.h"
#include "player.h"
#include <stdlib.h>
#include <time.h>

#define PIPE_CAP_HEIGHT 24   // pixels from top of pipe.png used as cap

// Assets
static void InitAssets(Game *game) {
    game->texBird = LoadTexture("assets/bird.png");
    game->texPipe = LoadTexture("assets/pipe.png");   // 80 x 217, cap at top
    game->texBg   = LoadTexture("assets/bg.png");

    InitAudioDevice();
    game->sFlap  = LoadSound("assets/sounds/flap.wav");
    game->sScore = LoadSound("assets/sounds/score.wav");
    game->sHit   = LoadSound("assets/sounds/hit.wav");

    // Load pixel font similar to Flappy Bird (press-start-2p or similar)
    game->font = LoadFont("assets/font.ttf");
}

// unload
static void UnloadAssets(Game *game) {
    UnloadTexture(game->texBird);
    UnloadTexture(game->texPipe);
    UnloadTexture(game->texBg);

    UnloadSound(game->sFlap);
    UnloadSound(game->sScore);
    UnloadSound(game->sHit);

    UnloadFont(game->font);

    CloseAudioDevice();
}

// Game lifecycle
void InitGame(Game *game) {
    srand((unsigned int)time(NULL));

    game->state = GAME_WAITING;
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
    game->state = GAME_WAITING;
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
    // Waiting: only start when player taps
    if (game->state == GAME_WAITING) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            game->state = GAME_RUNNING;
            BirdFlap(&game->bird);
            PlaySound(game->sFlap);
        }
        return;
    }

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

    // Colors for text
    Color uiColor = (Color){ 255, 230, 0, 255 };   // bright yellow
    Color shadow  = (Color){ 0, 0, 0, 160 };

    // Score text (with shadow for readability)
    int fontSizeScore = 32;
    Vector2 pos = { 20, 20 };
    Vector2 shadowOffset = { 2, 2 };

    const char *scoreStr = TextFormat("SCORE: %d", game->score);
    DrawTextEx(game->font, scoreStr,
               (Vector2){ pos.x + shadowOffset.x, pos.y + shadowOffset.y },
               fontSizeScore, 2, shadow);
    DrawTextEx(game->font, scoreStr, pos, fontSizeScore, 2, uiColor);

    int fontSizeBest = 20;
    Vector2 posBest = { 20, 60 };
    const char *bestStr = TextFormat("BEST: %d", game->highScore);
    DrawTextEx(game->font, bestStr,
               (Vector2){ posBest.x + shadowOffset.x, posBest.y + shadowOffset.y },
               fontSizeBest, 2, shadow);
    DrawTextEx(game->font, bestStr, posBest, fontSizeBest, 2, uiColor);

    // Waiting hint
    if (game->state == GAME_WAITING) {
        const char *hint = "Press SPACE or Left Click";
        int fontSize = 24;
        Vector2 textSize = MeasureTextEx(game->font, hint, fontSize, 2);
        Vector2 center = {
            SCREEN_WIDTH / 2.0f - textSize.x / 2.0f,
            SCREEN_HEIGHT / 2.0f
        };

        DrawTextEx(game->font, hint,
                   (Vector2){ center.x + shadowOffset.x
                DrawTextEx(game->font, hint,
                   (Vector2){ center.x + shadowOffset.x,
                              center.y + shadowOffset.y },
                   fontSize, 2, shadow);

        DrawTextEx(game->font, hint,
                   center,
                   fontSize, 2, uiColor);
    }

    // Game over overlay
    if (game->state == GAME_OVER) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

        const char *msg = "GAME OVER";
        int fontSize = 48;
        Vector2 msgSize = MeasureTextEx(game->font, msg, fontSize, 2);
        Vector2 msgPos = {
            SCREEN_WIDTH / 2.0f - msgSize.x / 2.0f,
            SCREEN_HEIGHT / 2.0f - 60.0f
        };

        // shadow + main text
        DrawTextEx(game->font, msg,
                   (Vector2){ msgPos.x + 2, msgPos.y + 2 },
                   fontSize, 2, shadow);
        DrawTextEx(game->font, msg, msgPos, fontSize, 2, uiColor);

        const char *hint = "Press SPACE or Left Click to restart";
        int hintSize = 20;
        Vector2 hintDim = MeasureTextEx(game->font, hint, hintSize, 2);
        Vector2 hintPos = {
            SCREEN_WIDTH / 2.0f - hintDim.x / 2.0f,
            SCREEN_HEIGHT / 2.0f + 10.0f
        };

        DrawTextEx(game->font, hint,
                   (Vector2){ hintPos.x + 2, hintPos.y + 2 },
                   hintSize, 2, shadow);
        DrawTextEx(game->font, hint, hintPos, hintSize, 2, uiColor);
    }
}