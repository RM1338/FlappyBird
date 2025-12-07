// main.c

#include "raylib.h"
#include "game.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird");
    SetTargetFPS(60);
    
    // Disable ESC key to close window - we handle ESC ourselves
    SetExitKey(KEY_NULL);

    Game game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Always call UpdateGame – it handles WAITING/RUNNING/OVER/PAUSED/SETTINGS
        UpdateGame(&game, dt);

        BeginDrawing();
        ClearBackground(RAYWHITE);    // bg texture will cover this
        DrawGame(&game);
        EndDrawing();
    }

    UnloadGame(&game);
    CloseWindow();
    return 0;
}