#include "raylib.h"
#include "game.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird");
    SetTargetFPS(60);

    Game game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Always call UpdateGame – it handles WAITING/RUNNING/OVER
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