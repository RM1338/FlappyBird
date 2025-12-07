// src/game.c
#include "game.h"
#include "player.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#define PIPE_CAPACITY   16
#define PIPE_CAP_HEIGHT 24

// --- HELPER FUNCTION PROTOTYPES ---
static void DrawWaitingScreen(const Game *game);
static void DrawGameOverScreen(const Game *game);
static void DrawPauseScreen(const Game *game);
static void DrawSettingsScreen(const Game *game);
static void DrawLives(const Game *game);
static void DrawRespawnCountdown(const Game *game);

// --- SETTINGS ---
void SaveSettings(const Settings *settings) {
    FILE *file = fopen("settings.dat", "wb");
    if (file) {
        fwrite(settings, sizeof(Settings), 1, file);
        fclose(file);
    }
}

void LoadSettings(Settings *settings) {
    FILE *file = fopen("settings.dat", "rb");
    if (file) {
        fread(settings, sizeof(Settings), 1, file);
        fclose(file);
    } else {
        // Default settings
        settings->musicVolume = 0.5f;
        settings->sfxVolume = 0.7f;
        settings->screenShake = true;
    }
}

// --- SCREEN SHAKE ---
void ApplyScreenShake(Game *game, float magnitude, float duration) {
    if (game->settings.screenShake) {
        game->shakeMagnitude = magnitude;
        game->shakeTimer = duration;
    }
}

void UpdateScreenShake(Game *game, float dt) {
    if (game->shakeTimer > 0.0f) {
        game->shakeTimer -= dt;
        
        // Random shake offset
        float angle = (float)(rand() % 360) * DEG2RAD;
        float intensity = game->shakeMagnitude * (game->shakeTimer / 0.3f); // Fade out
        
        game->shakeOffset.x = cosf(angle) * intensity;
        game->shakeOffset.y = sinf(angle) * intensity;
    } else {
        game->shakeOffset.x = 0.0f;
        game->shakeOffset.y = 0.0f;
    }
}

// --- ASSETS ---
static void InitAssets(Game *game) {
    game->texBird = LoadTexture("assets/bird.png");
    game->texPipe = LoadTexture("assets/pipe.png");
    game->texBg   = LoadTexture("assets/bg.png");

    InitAudioDevice();
    game->sFlap  = LoadSound("assets/sounds/flap.wav");
    game->sScore = LoadSound("assets/sounds/score.wav");
    game->sHit   = LoadSound("assets/sounds/hit.wav");

    // Load font with proper size for better rendering
    game->font = LoadFontEx("assets/font.ttf", 64, NULL, 0);
    
    // Apply volume settings
    SetSoundVolume(game->sFlap, game->settings.sfxVolume);
    SetSoundVolume(game->sScore, game->settings.sfxVolume);
    SetSoundVolume(game->sHit, game->settings.sfxVolume);
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
    game->lives = MAX_LIVES;
    game->pipeSpawnTimer = 0.0f;
    game->pipeCount = 0;
    game->respawnTimer = 0.0f;
    
    game->shakeTimer = 0.0f;
    game->shakeMagnitude = 0.0f;
    game->shakeOffset = (Vector2){0, 0};

    LoadSettings(&game->settings);
    InitAssets(game);
    InitBird(&game->bird);

    for (int i = 0; i < PIPE_CAPACITY; i++) {
        game->pipes[i].active = false;
        game->pipes[i].scored = false;
    }

    SpawnPipe(game);
}

void ResetGame(Game *game) {
    game->state = GAME_RUNNING;
    game->score = 0;
    game->lives = MAX_LIVES;
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
    SaveSettings(&game->settings);
    UnloadAssets(game);
}

// --- PIPES ---
void SpawnPipe(Game *game) {
    Pipe *pipe = NULL;
    int foundIndex = -1;

    for (int i = 0; i < PIPE_CAPACITY; i++) {
        int checkIndex = (game->pipeCount + i) % PIPE_CAPACITY;
        if (!game->pipes[checkIndex].active) {
            pipe = &game->pipes[checkIndex];
            foundIndex = checkIndex;
            break;
        }
    }

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
    UpdateScreenShake(game, dt);
    
    // RESPAWN COUNTDOWN STATE
    if (game->state == GAME_RESPAWN_COUNTDOWN) {
        game->respawnTimer -= dt;
        
        if (game->respawnTimer <= 0.0f) {
            game->state = GAME_RUNNING;
        }
        return;
    }
    
    // WAITING STATE
    if (game->state == GAME_WAITING) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            game->state = GAME_RUNNING;
            BirdFlap(&game->bird);
            PlaySound(game->sFlap);
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->state = GAME_SETTINGS;
        }
        return;
    }

    // GAME OVER STATE
    if (game->state == GAME_OVER) {
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            ResetGame(game);
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->state = GAME_WAITING;
        }
        return;
    }
    
    // PAUSED STATE
    if (game->state == GAME_PAUSED) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            game->state = GAME_RUNNING;
        }
        if (IsKeyPressed(KEY_S)) {
            game->state = GAME_SETTINGS;
        }
        return;
    }
    
    // SETTINGS STATE
    if (game->state == GAME_SETTINGS) {
        // Volume controls
        if (IsKeyDown(KEY_UP)) {
            game->settings.sfxVolume += 0.01f;
            if (game->settings.sfxVolume > 1.0f) game->settings.sfxVolume = 1.0f;
            SetSoundVolume(game->sFlap, game->settings.sfxVolume);
            SetSoundVolume(game->sScore, game->settings.sfxVolume);
            SetSoundVolume(game->sHit, game->settings.sfxVolume);
        }
        if (IsKeyDown(KEY_DOWN)) {
            game->settings.sfxVolume -= 0.01f;
            if (game->settings.sfxVolume < 0.0f) game->settings.sfxVolume = 0.0f;
            SetSoundVolume(game->sFlap, game->settings.sfxVolume);
            SetSoundVolume(game->sScore, game->settings.sfxVolume);
            SetSoundVolume(game->sHit, game->settings.sfxVolume);
        }
        
        // Toggle screen shake
        if (IsKeyPressed(KEY_T)) {
            game->settings.screenShake = !game->settings.screenShake;
        }
        
        // Back to menu
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
            SaveSettings(&game->settings);
            game->state = GAME_WAITING;
        }
        return;
    }

    // RUNNING STATE
    if (game->state != GAME_RUNNING) return;
    
    // Pause
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        game->state = GAME_PAUSED;
        return;
    }

    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        BirdFlap(&game->bird);
        PlaySound(game->sFlap);
    }

    UpdateBird(&game->bird, dt);

    if (BirdHitWorld(&game->bird)) {
        game->lives--;
        ApplyScreenShake(game, 10.0f, 0.3f);
        PlaySound(game->sHit);
        
        if (game->lives <= 0) {
            game->state = GAME_OVER;
            if (game->score > game->highScore) {
                game->highScore = game->score;
            }
        } else {
            // Reset bird position and start countdown
            InitBird(&game->bird);
            game->respawnTimer = 3.0f;
            game->state = GAME_RESPAWN_COUNTDOWN;
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
            game->lives--;
            ApplyScreenShake(game, 10.0f, 0.3f);
            PlaySound(game->sHit);
            
            if (game->lives <= 0) {
                game->state = GAME_OVER;
                if (game->score > game->highScore) {
                    game->highScore = game->score;
                }
            } else {
                // Reset bird and clear pipes, then start countdown
                InitBird(&game->bird);
                for (int j = 0; j < PIPE_CAPACITY; j++) {
                    game->pipes[j].active = false;
                }
                game->pipeSpawnTimer = 0.0f;
                SpawnPipe(game);
                game->respawnTimer = 3.0f;
                game->state = GAME_RESPAWN_COUNTDOWN;
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
static void DrawLives(const Game *game) {
    Color heartColor = RED;
    int heartSize = 20;
    int spacing = 30;
    int startX = SCREEN_WIDTH - (MAX_LIVES * spacing) - 10;
    int startY = 10;
    
    for (int i = 0; i < MAX_LIVES; i++) {
        Color color = (i < game->lives) ? heartColor : GRAY;
        int x = startX + (i * spacing);
        
        // Draw simple heart shape using triangles
        DrawCircle(x, startY + 5, 8, color);
        DrawCircle(x + 12, startY + 5, 8, color);
        DrawTriangle(
            (Vector2){x - 8, startY + 5},
            (Vector2){x + 20, startY + 5},
            (Vector2){x + 6, startY + 22},
            color
        );
    }
}

static void DrawRespawnCountdown(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
    
    bool useCustomFont = (game->font.texture.id > 0);
    Color yellow = (Color){ 255, 230, 0, 255 };
    Color shadow = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 3, 3 };
    
    int countdown = (int)ceilf(game->respawnTimer);
    const char *text;
    
    if (countdown == 3) text = "3";
    else if (countdown == 2) text = "2";
    else if (countdown == 1) text = "1";
    else text = "GO!";
    
    float fontSize = 72.0f;
    
    if (useCustomFont) {
        Vector2 textDim = MeasureTextEx(game->font, text, fontSize, 2.0f);
        float textX = (SCREEN_WIDTH - textDim.x) / 2.0f;
        float textY = (SCREEN_HEIGHT - textDim.y) / 2.0f;
        
        DrawTextEx(game->font, text,
                   (Vector2){ textX + shadowOffset.x, textY + shadowOffset.y },
                   fontSize, 2.0f, shadow);
        DrawTextEx(game->font, text, (Vector2){ textX, textY }, fontSize, 2.0f, yellow);
    } else {
        int textWidth = MeasureText(text, (int)fontSize);
        int textX = (SCREEN_WIDTH - textWidth) / 2;
        int textY = SCREEN_HEIGHT / 2 - (int)fontSize / 2;
        
        DrawText(text, textX + (int)shadowOffset.x, textY + (int)shadowOffset.y, (int)fontSize, shadow);
        DrawText(text, textX, textY, (int)fontSize, yellow);
    }
    
    // Show remaining lives message
    const char *lifeMsg = TextFormat("LIVES REMAINING: %d", game->lives);
    float msgSize = 24.0f;
    
    if (useCustomFont) {
        Vector2 msgDim = MeasureTextEx(game->font, lifeMsg, msgSize, 2.0f);
        float msgX = (SCREEN_WIDTH - msgDim.x) / 2.0f;
        DrawTextEx(game->font, lifeMsg, (Vector2){ msgX, SCREEN_HEIGHT / 2.0f + 80 }, msgSize, 2.0f, WHITE);
    } else {
        int msgWidth = MeasureText(lifeMsg, (int)msgSize);
        int msgX = (SCREEN_WIDTH - msgWidth) / 2;
        DrawText(lifeMsg, msgX, SCREEN_HEIGHT / 2 + 80, (int)msgSize, WHITE);
    }
}

static void DrawWaitingScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));

    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    bool useCustomFont = (game->font.texture.id > 0);

    const char *title = "FLAPPY BIRD";
    float titleSize = 48.0f;
    float titleX, titleY;
    
    if (useCustomFont) {
        float titleSpacing = 2.0f;
        Vector2 titleDim = MeasureTextEx(game->font, title, titleSize, titleSpacing);
        titleX = (SCREEN_WIDTH - titleDim.x) / 2.0f;
        titleY = SCREEN_HEIGHT / 2.0f - 100.0f;
        
        DrawTextEx(game->font, title,
                   (Vector2){ titleX + shadowOffset.x, titleY + shadowOffset.y },
                   titleSize, titleSpacing, shadow);
        DrawTextEx(game->font, title, (Vector2){ titleX, titleY }, titleSize, titleSpacing, uiColor);
    } else {
        int titleWidth = MeasureText(title, (int)titleSize);
        titleX = (SCREEN_WIDTH - titleWidth) / 2.0f;
        titleY = SCREEN_HEIGHT / 2.0f - 100.0f;
        
        DrawText(title, (int)(titleX + shadowOffset.x), (int)(titleY + shadowOffset.y), (int)titleSize, shadow);
        DrawText(title, (int)titleX, (int)titleY, (int)titleSize, uiColor);
    }

    const char *hint = "Press SPACE or Click to Start";
    const char *settings = "Press ESC for Settings";
    float hintSize = 24.0f;
    float hintX, hintY;
    
    if (useCustomFont) {
        float hintSpacing = 2.0f;
        Vector2 hintDim = MeasureTextEx(game->font, hint, hintSize, hintSpacing);
        hintX = (SCREEN_WIDTH - hintDim.x) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawTextEx(game->font, hint, (Vector2){ hintX + 1, hintY + 1 }, hintSize, hintSpacing, shadow);
        DrawTextEx(game->font, hint, (Vector2){ hintX, hintY }, hintSize, hintSpacing, WHITE);
        
        Vector2 settingsDim = MeasureTextEx(game->font, settings, 20.0f, hintSpacing);
        float settingsX = (SCREEN_WIDTH - settingsDim.x) / 2.0f;
        DrawTextEx(game->font, settings, (Vector2){ settingsX, hintY + 40 }, 20.0f, hintSpacing, GRAY);
    } else {
        int hintWidth = MeasureText(hint, (int)hintSize);
        hintX = (SCREEN_WIDTH - hintWidth) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawText(hint, (int)(hintX + 1), (int)(hintY + 1), (int)hintSize, shadow);
        DrawText(hint, (int)hintX, (int)hintY, (int)hintSize, WHITE);
        
        int settingsWidth = MeasureText(settings, 20);
        int settingsX = (SCREEN_WIDTH - settingsWidth) / 2;
        DrawText(settings, settingsX, (int)hintY + 40, 20, GRAY);
    }
}

static void DrawGameOverScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    bool useCustomFont = (game->font.texture.id > 0);

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

    const char *hint = "Press SPACE or Click to Restart";
    const char *menu = "Press ESC for Menu";
    float hintSize = 24.0f;
    float hintX, hintY;
    
    if (useCustomFont) {
        float hintSpacing = 2.0f;
        Vector2 hintSizeVec = MeasureTextEx(game->font, hint, hintSize, hintSpacing);
        hintX = (SCREEN_WIDTH - hintSizeVec.x) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawTextEx(game->font, hint, (Vector2){ hintX + 1, hintY + 1 }, hintSize, hintSpacing, shadow);
        DrawTextEx(game->font, hint, (Vector2){ hintX, hintY }, hintSize, hintSpacing, WHITE);
        
        Vector2 menuDim = MeasureTextEx(game->font, menu, 20.0f, hintSpacing);
        float menuX = (SCREEN_WIDTH - menuDim.x) / 2.0f;
        DrawTextEx(game->font, menu, (Vector2){ menuX, hintY + 40 }, 20.0f, hintSpacing, GRAY);
    } else {
        int hintWidth = MeasureText(hint, (int)hintSize);
        hintX = (SCREEN_WIDTH - hintWidth) / 2.0f;
        hintY = SCREEN_HEIGHT / 2.0f + 10.0f;
        
        DrawText(hint, (int)(hintX + 1), (int)(hintY + 1), (int)hintSize, shadow);
        DrawText(hint, (int)hintX, (int)hintY, (int)hintSize, WHITE);
        
        int menuWidth = MeasureText(menu, 20);
        int menuX = (SCREEN_WIDTH - menuWidth) / 2;
        DrawText(menu, menuX, (int)hintY + 40, 20, GRAY);
    }
}

static void DrawPauseScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));
    
    bool useCustomFont = (game->font.texture.id > 0);
    Color yellow = (Color){ 255, 230, 0, 255 };
    
    const char *title = "PAUSED";
    const char *resume = "Press ESC or P to Resume";
    const char *settings = "Press S for Settings";
    
    if (useCustomFont) {
        Vector2 titleDim = MeasureTextEx(game->font, title, 48.0f, 2.0f);
        float titleX = (SCREEN_WIDTH - titleDim.x) / 2.0f;
        DrawTextEx(game->font, title, (Vector2){ titleX, SCREEN_HEIGHT / 2.0f - 50 }, 48.0f, 2.0f, yellow);
        
        Vector2 resumeDim = MeasureTextEx(game->font, resume, 24.0f, 2.0f);
        float resumeX = (SCREEN_WIDTH - resumeDim.x) / 2.0f;
        DrawTextEx(game->font, resume, (Vector2){ resumeX, SCREEN_HEIGHT / 2.0f + 20 }, 24.0f, 2.0f, WHITE);
        
        Vector2 settingsDim = MeasureTextEx(game->font, settings, 20.0f, 2.0f);
        float settingsX = (SCREEN_WIDTH - settingsDim.x) / 2.0f;
        DrawTextEx(game->font, settings, (Vector2){ settingsX, SCREEN_HEIGHT / 2.0f + 60 }, 20.0f, 2.0f, GRAY);
    } else {
        int titleWidth = MeasureText(title, 48);
        DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, SCREEN_HEIGHT / 2 - 50, 48, yellow);
        
        int resumeWidth = MeasureText(resume, 24);
        DrawText(resume, (SCREEN_WIDTH - resumeWidth) / 2, SCREEN_HEIGHT / 2 + 20, 24, WHITE);
        
        int settingsWidth = MeasureText(settings, 20);
        DrawText(settings, (SCREEN_WIDTH - settingsWidth) / 2, SCREEN_HEIGHT / 2 + 60, 20, GRAY);
    }
}

static void DrawSettingsScreen(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    
    bool useCustomFont = (game->font.texture.id > 0);
    Color yellow = (Color){ 255, 230, 0, 255 };
    
    const char *title = "SETTINGS";
    const char *volumeText = TextFormat("SFX Volume: %.0f%%", game->settings.sfxVolume * 100);
    const char *shakeText = TextFormat("Screen Shake: %s", game->settings.screenShake ? "ON" : "OFF");
    const char *controls1 = "UP/DOWN - Adjust Volume";
    const char *controls2 = "T - Toggle Screen Shake";
    const char *back = "Press ESC or ENTER to Save & Exit";
    
    int yPos = 150;
    
    if (useCustomFont) {
        Vector2 titleDim = MeasureTextEx(game->font, title, 48.0f, 2.0f);
        float titleX = (SCREEN_WIDTH - titleDim.x) / 2.0f;
        DrawTextEx(game->font, title, (Vector2){ titleX, 100 }, 48.0f, 2.0f, yellow);
        
        DrawTextEx(game->font, volumeText, (Vector2){ 100, yPos }, 24.0f, 2.0f, WHITE);
        DrawTextEx(game->font, shakeText, (Vector2){ 100, yPos + 50 }, 24.0f, 2.0f, WHITE);
        
        DrawTextEx(game->font, controls1, (Vector2){ 100, yPos + 120 }, 18.0f, 2.0f, GRAY);
        DrawTextEx(game->font, controls2, (Vector2){ 100, yPos + 150 }, 18.0f, 2.0f, GRAY);
        
        Vector2 backDim = MeasureTextEx(game->font, back, 20.0f, 2.0f);
        float backX = (SCREEN_WIDTH - backDim.x) / 2.0f;
        DrawTextEx(game->font, back, (Vector2){ backX, SCREEN_HEIGHT - 80 }, 20.0f, 2.0f, YELLOW);
    } else {
        int titleWidth = MeasureText(title, 48);
        DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, 100, 48, yellow);
        
        DrawText(volumeText, 100, yPos, 24, WHITE);
        DrawText(shakeText, 100, yPos + 50, 24, WHITE);
        
        DrawText(controls1, 100, yPos + 120, 18, GRAY);
        DrawText(controls2, 100, yPos + 150, 18, GRAY);
        
        int backWidth = MeasureText(back, 20);
        DrawText(back, (SCREEN_WIDTH - backWidth) / 2, SCREEN_HEIGHT - 80, 20, YELLOW);
    }
    
    // Volume bar
    DrawRectangle(100, yPos + 25, 300, 10, DARKGRAY);
    DrawRectangle(100, yPos + 25, (int)(300 * game->settings.sfxVolume), 10, GREEN);
}

// --- DRAW ---
void DrawGame(const Game *game) {
    // Apply screen shake offset
    BeginMode2D((Camera2D){ game->shakeOffset, (Vector2){0, 0}, 0.0f, 1.0f });
    
    DrawTexture(game->texBg, 0, 0, WHITE);

    // Pipe texture regions
    Rectangle srcCap        = { 0, 0, PIPE_WIDTH, PIPE_CAP_HEIGHT };
    Rectangle srcCapFlipped = { 0, PIPE_CAP_HEIGHT, PIPE_WIDTH, -PIPE_CAP_HEIGHT };

    float pipeBodyHeight = game->texPipe.height - PIPE_CAP_HEIGHT;
    Rectangle srcBody        = { 0, PIPE_CAP_HEIGHT, PIPE_WIDTH, pipeBodyHeight };
    Rectangle srcBodyFlipped = { 0, game->texPipe.height, PIPE_WIDTH, -pipeBodyHeight };

    // Draw pipes
    for (int i = 0; i < PIPE_CAPACITY; i++) {
        const Pipe *p = &game->pipes[i];
        if (!p->active) continue;

        float topBodyHeight = p->top.height - PIPE_CAP_HEIGHT;
        if (topBodyHeight < 0) topBodyHeight = 0;

        Rectangle dstTopBody = { p->top.x, p->top.y, PIPE_WIDTH, topBodyHeight };
        DrawTexturePro(game->texPipe, srcBody, dstTopBody, (Vector2){0,0}, 0.0f, WHITE);

        Rectangle dstTopCap = { p->top.x, p->top.y + topBodyHeight, PIPE_WIDTH, PIPE_CAP_HEIGHT };
        DrawTexturePro(game->texPipe, srcCap, dstTopCap, (Vector2){0,0}, 0.0f, WHITE);

        float bottomBodyHeight = p->bottom.height - PIPE_CAP_HEIGHT;
        if (bottomBodyHeight < 0) bottomBodyHeight = 0;

        Rectangle dstBottomCap = { p->bottom.x, p->bottom.y, PIPE_WIDTH, PIPE_CAP_HEIGHT };
        DrawTexturePro(game->texPipe, srcCapFlipped, dstBottomCap, (Vector2){0,0}, 0.0f, WHITE);

        Rectangle dstBottomBody = { p->bottom.x, p->bottom.y + PIPE_CAP_HEIGHT, PIPE_WIDTH, bottomBodyHeight };
        DrawTexturePro(game->texPipe, srcBodyFlipped, dstBottomBody, (Vector2){0,0}, 0.0f, WHITE);
    }

    DrawBirdSprite(game);
    
    EndMode2D();

    // UI (not affected by shake)
    Color uiColor = (Color){ 255, 230, 0, 255 };
    Color shadow  = (Color){ 0, 0, 0, 160 };
    Vector2 shadowOffset = { 2, 2 };

    bool useCustomFont = (game->font.texture.id > 0);
    
    const char *scoreStr = TextFormat("SCORE: %d", game->score);
    const char *bestStr = TextFormat("BEST: %d", game->highScore);

    if (useCustomFont) {
        DrawTextEx(game->font, scoreStr, (Vector2){ 20 + shadowOffset.x, 20 + shadowOffset.y }, 32, 2, shadow);
        DrawTextEx(game->font, scoreStr, (Vector2){ 20, 20 }, 32, 2, uiColor);

        DrawTextEx(game->font, bestStr, (Vector2){ 20 + shadowOffset.x, 60 + shadowOffset.y }, 20, 2, shadow);
        DrawTextEx(game->font, bestStr, (Vector2){ 20, 60 }, 20, 2, uiColor);
    } else {
        DrawText(scoreStr, 20 + (int)shadowOffset.x, 20 + (int)shadowOffset.y, 32, shadow);
        DrawText(scoreStr, 20, 20, 32, uiColor);
        
        DrawText(bestStr, 20 + (int)shadowOffset.x, 60 + (int)shadowOffset.y, 20, shadow);
        DrawText(bestStr, 20, 60, 20, uiColor);
    }
    
    // Draw lives (hearts)
    DrawLives(game);

    // Overlays
    if (game->state == GAME_WAITING) {
        DrawWaitingScreen(game);
    }

    if (game->state == GAME_OVER) {
        DrawGameOverScreen(game);
    }
    
    if (game->state == GAME_PAUSED) {
        DrawPauseScreen(game);
    }
    
    if (game->state == GAME_SETTINGS) {
        DrawSettingsScreen(game);
    }
    
    if (game->state == GAME_RESPAWN_COUNTDOWN) {
        DrawRespawnCountdown(game);
    }
}