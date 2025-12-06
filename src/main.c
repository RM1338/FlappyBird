// src/main.c
#include "raylib.h"
#include "game.h"

int main(void) {
    // Create window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird");
    SetTargetFPS(60);

    // Create and initialize game
    Game game;
    InitGame(&game);

    // Main loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Update
        if (game.state == GAME_RUNNING) {
            UpdateGame(&game, dt);
        } else {
            // GAME_OVER: restart on SPACE or mouse click
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                ResetGame(&game);
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);   // will be fully covered by bg texture
        DrawGame(&game);
        EndDrawing();
    }

    // Cleanup
    UnloadGame(&game);
    CloseWindow();

    return 0;
}