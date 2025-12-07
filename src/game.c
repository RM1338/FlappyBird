// src/game.c
#include "game.h"
#include "player.h"
#include <stdlib.h>
#include <time.h>

#define PIPE_CAPACITY   16
#define PIPE_CAP_HEIGHT 24

// --- HELPER FUNCTION PROTOTYPES ---
static void DrawWaitingScreen(const Game *game);
static void DrawGameOverScreen(const Game *game);

// --- ASSETS ---
static void InitAssets(Game *game) {
    game->texBird = LoadTexture("assets/bird.png");
    game->texPipe = LoadTexture("assets/pipe.png");   // 80x217, cap at top
    game->texBg   = LoadTexture("assets/bg.png");

    InitAudioDevice();
    game->sFlap  = LoadSound("assets/sounds/flap.wav");
    game->sScore = LoadSound("assets/sounds/score.wav");
    game->sHit   = LoadSound("assets/sounds/hit.wav");

    game->font = LoadFont("assets/font.ttf");
}

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

// --- GAME LIFECYCLE ---
void InitGame(Game *game) {
    srand((unsigned int)time(NULL));

    game->state = GAME_WAITING;
    game->score = 0;
    game->highScore = 0;
    game->pipeSpawnTimer = 0.0f;
    game->pipeCount = 0;

    InitAssets(game);
    InitBird(&game->bird);

    for (int i = 0; i < PIPE_CAPACITY; i++) {
        game->pipes[i].active = false;
        game->pipes[i].scored = false;
    }

    SpawnPipe(game);
}

void ResetGame(Game *game) {
    game->state = GAME_RUNNING;            // restart directly
    game->score = 0;
    game->pipeSpawnTimer = 0.0f;
    game->pipeCount = 0;

    InitBird(&game->bird);

    for (int i = 0; i < PIPE_CAPACITY; i++) {
        game->pipes[i].active = false;
        game->pipes[i].scored = false;
    }

    SpawnPipe(game);
}

void UnloadGame(Game *game) {
    UnloadAssets(game);
}

// --- PIPES ---
void SpawnPipe(Game *game) {
    Pipe *pipe = NULL;
    int foundIndex = -1;

    // find inactive slot
    for (int i = 0; i < PIPE_CAPACITY; i++) {
        int checkIndex = (game->pipeCount + i) % PIPE_CAPACITY;
        if (!game->pipes[checkIndex].active) {
            pipe = &game->pipes[checkIndex];
            foundIndex = checkIndex;
            break;
        }
    }

    // if none, overwrite oldest
    if (pipe == NULL) {
        int overwriteIndex = game->pipeCount % PIPE_CAPACITY;
        pipe = &game->pipes[overwriteIndex];
        foundIndex = overwriteIndex;
    }

    game->pipeCount = (foundIndex + 1) % PIPE_CAPACITY;

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
}

// --- UPDATE ---
void UpdateGame(Game *game, float dt) {
    // WAITING: first run only
    if (game->state == GAME_WAITING) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            game->state = GAME_RUNNING;
            BirdFlap(&game->bird);
            PlaySound(game->sFlap);
        }
        return;
    }

    // GAME OVER: tap to restart
    if (game->state == GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ResetGame(game);
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
        if (game->score > game->highScore) {
            game->highScore = game->score;
        }
        return;
    }

    Rectangle birdRect = BirdGetRect(&game->bird);

    for (int i = 0; i < PIPE_CAPACITY; i++) {
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
            if (game->score > game->highScore) {
                game->highScore = game->score;
            }
            return;
        }

        float pipeCenterX = p->top.x + PIPE_WIDTH * 0.5f;
        if (!p->scored && birdRect.x > pipeCenterX) {
            p->scored = true;
            game->score++;
            PlaySound(game->sScore);
        }
    }

    game->pipeSpawnTimer += dt;
    if (game->pipeSpawnTimer >= PIPE_SPAWN_TIME) {
        game->pipeSpawnTimer = 0.0f;
        SpawnPipe(game);
    }
}

// --- DRAWING HELPERS ---
static void DrawWaitingScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));

    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    // Check if custom font is loaded (font.id != 0 means custom font loaded)
    bool useCustomFont = (game->font.texture.id > 0);

    // Title
    const char *title = "FLAPPY BIRD";
    float titleSize = 48.0f;
    float titleX, titleY;
    
    if (useCustomFont) {
        float titleSpacing = 2.0f;
        Vector2 titleDim = MeasureTextEx(game->font, title, titleSize, titleSpacing);
        titleX = (SCREEN_WIDTH - titleDim.x) / 2.0f;
        titleY = SCREEN_HEIGHT / 2.0f - 80.0f;
        
        DrawTextEx(game->font, title,
                   (Vector2){ titleX + shadowOffset.x, titleY + shadowOffset.y },
                   titleSize, titleSpacing, shadow);
        DrawTextEx(game->font, title, (Vector2){ titleX, titleY }, titleSize, titleSpacing, uiColor);
    } else {
        // Use default font with MeasureText
        int titleWidth = MeasureText(title, (int)titleSize);
        titleX = (SCREEN_WIDTH - titleWidth) / 2.0f;
        titleY = SCREEN_HEIGHT / 2.0f - 80.0f;
        
        DrawText(title, (int)(titleX + shadowOffset.x), (int)(titleY + shadowOffset.y), (int)titleSize, shadow);
        DrawText(title, (int)titleX, (int)titleY, (int)titleSize, uiColor);
    }

    // Hint
    const char *hint = "Press SPACE or Click to Start";
    float hintSize = 24.0f;
    float hintX, hintY;
    
    if (useCustomFont) {
        float hintSpacing = 2.0f;
        Vector2 hintDim = MeasureTextEx(game->font, hint, hintSize, hintSpacing);
        hintX = (SCREEN_WIDTH - hintDim.x) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawTextEx(game->font, hint,
                   (Vector2){ hintX + 1, hintY + 1 },
                   hintSize, hintSpacing, shadow);
        DrawTextEx(game->font, hint, (Vector2){ hintX, hintY }, hintSize, hintSpacing, WHITE);
    } else {
        int hintWidth = MeasureText(hint, (int)hintSize);
        hintX = (SCREEN_WIDTH - hintWidth) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawText(hint, (int)(hintX + 1), (int)(hintY + 1), (int)hintSize, shadow);
        DrawText(hint, (int)hintX, (int)hintY, (int)hintSize, WHITE);
    }
}

static void DrawGameOverScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    bool useCustomFont = (game->font.texture.id > 0);

    // Game Over Message
    const char *msg = "GAME OVER";
    float fontSize = 48.0f;
    float msgX, msgY;
    
    if (useCustomFont) {
        float fontSpacing = 2.0f;
        Vector2 msgSize = MeasureTextEx(game->font, msg, fontSize, fontSpacing);
        msgX = (SCREEN_WIDTH - msgSize.x) / 2.0f;
        msgY = SCREEN_HEIGHT / 2.0f - 60.0f;
        
        DrawTextEx(game->font, msg,
                   (Vector2){ msgX + shadowOffset.x, msgY + shadowOffset.y },
                   fontSize, fontSpacing, shadow);
        DrawTextEx(game->font, msg, (Vector2){ msgX, msgY }, fontSize, fontSpacing, uiColor);
    } else {
        int msgWidth = MeasureText(msg, (int)fontSize);
        msgX = (SCREEN_WIDTH - msgWidth) / 2.0f;
        msgY = SCREEN_HEIGHT / 2.0f - 60.0f;
        
        DrawText(msg, (int)(msgX + shadowOffset.x), (int)(msgY + shadowOffset.y), (int)fontSize, shadow);
        DrawText(msg, (int)msgX, (int)msgY, (int)fontSize, uiColor);
    }

    // Restart Hint
    const char *hint = "Press SPACE or Click to Restart";
    float hintSize = 24.0f;
    float hintX, hintY;
    
    if (useCustomFont) {
        float hintSpacing = 2.0f;
        Vector2 hintSizeVec = MeasureTextEx(game->font, hint, hintSize, hintSpacing);
        hintX = (SCREEN_WIDTH - hintSizeVec.x) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawTextEx(game->font, hint,
                   (Vector2){ hintX + 1, hintY + 1 },
                   hintSize, hintSpacing, shadow);
        DrawTextEx(game->font, hint, (Vector2){ hintX, hintY }, hintSize, hintSpacing, WHITE);
    } else {
        int hintWidth = MeasureText(hint, (int)hintSize);
        hintX = (SCREEN_WIDTH - hintWidth) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawText(hint, (int)(hintX + 1), (int)(hintY + 1), (int)hintSize, shadow);
        DrawText(hint, (int)hintX, (int)hintY, (int)hintSize, WHITE);
    }
}

// --- DRAW ---
void DrawGame(const Game *game) {
    DrawTexture(game->texBg, 0, 0, WHITE);

    // Pipe texture regions (pipe.png 80x217, cap at top)
    Rectangle srcCap        = { 0, 0, PIPE_WIDTH, PIPE_CAP_HEIGHT };
    Rectangle srcCapFlipped = { 0, PIPE_CAP_HEIGHT, PIPE_WIDTH, -PIPE_CAP_HEIGHT };

    float pipeBodyHeight = game->texPipe.height - PIPE_CAP_HEIGHT;
    Rectangle srcBody        = { 0, PIPE_CAP_HEIGHT, PIPE_WIDTH, pipeBodyHeight };
    Rectangle srcBodyFlipped = { 0, game->texPipe.height, PIPE_WIDTH, -pipeBodyHeight };

    // Draw pipes
    for (int i = 0; i < PIPE_CAPACITY; i++) {
        const Pipe *p = &game->pipes[i];
        if (!p->active) continue;

        // Top pipe body
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

        // Top pipe cap
        Rectangle dstTopCap = {
            p->top.x,
            p->top.y + topBodyHeight,
            PIPE_WIDTH,
            PIPE_CAP_HEIGHT
        };
        DrawTexturePro(game->texPipe, srcCap, dstTopCap,
                       (Vector2){0,0}, 0.0f, WHITE);

        // Bottom pipe cap
        float bottomBodyHeight = p->bottom.height - PIPE_CAP_HEIGHT;
        if (bottomBodyHeight < 0) bottomBodyHeight = 0;

        Rectangle dstBottomCap = {
            p->bottom.x,
            p->bottom.y,
            PIPE_WIDTH,
            PIPE_CAP_HEIGHT
        };
        DrawTexturePro(game->texPipe, srcCapFlipped, dstBottomCap,
                       (Vector2){0,0}, 0.0f, WHITE);

        // Bottom pipe body
        Rectangle dstBottomBody = {
            p->bottom.x,
            p->bottom.y + PIPE_CAP_HEIGHT,
            PIPE_WIDTH,
            bottomBodyHeight
        };
        DrawTexturePro(game->texPipe, srcBodyFlipped, dstBottomBody,
                       (Vector2){0,0}, 0.0f, WHITE);
    }

    // Bird
    DrawBirdSprite(game);

    // Score UI
    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    bool useCustomFont = (game->font.texture.id > 0);
    
    const char *scoreStr = TextFormat("SCORE: %d", game->score);
    const char *bestStr = TextFormat("BEST: %d", game->highScore);

    if (useCustomFont) {
        int fontSizeScore = 32;
        Vector2 posScore = { 20, 20 };
        DrawTextEx(game->font, scoreStr,
                   (Vector2){ posScore.x + shadowOffset.x, posScore.y + shadowOffset.y },
                   (float)fontSizeScore, 2, shadow);
        DrawTextEx(game->font, scoreStr, posScore, (float)fontSizeScore, 2, uiColor);

        int fontSizeBest = 20;
        Vector2 posBest = { 20, 60 };
        DrawTextEx(game->font, bestStr,
                   (Vector2){ posBest.x + shadowOffset.x, posBest.y + shadowOffset.y },
                   (float)fontSizeBest, 2, shadow);
        DrawTextEx(game->font, bestStr, posBest, (float)fontSizeBest, 2, uiColor);
    } else {
        DrawText(scoreStr, 20 + (int)shadowOffset.x, 20 + (int)shadowOffset.y, 32, shadow);
        DrawText(scoreStr, 20, 20, 32, uiColor);
        
        DrawText(bestStr, 20 + (int)shadowOffset.x, 60 + (int)shadowOffset.y, 20, shadow);
        DrawText(bestStr, 20, 60, 20, uiColor);
    }

    // Overlays
    if (game->state == GAME_WAITING) {
        DrawWaitingScreen(game);
    }

    if (game->state == GAME_OVER) {
        DrawGameOverScreen(game);
    }
}