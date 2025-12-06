#include "player.h"

#define BIRD_WIDTH 27
#define BIRD_HEIGHT 20

void InitBird(Bird *bird) {
    bird->position = (Vector2){ 100.0f, SCREEN_HEIGHT / 2.0f };
    bird->size = (Vector2){ BIRD_WIDTH, BIRD_HEIGHT };
    bird->velocity = 0.0f;
    bird->acceleration = GRAVITY;
    bird->frame = 0;
    bird->frameTimer = 0.0f;
}

void UpdateBird(Bird *bird, float dt) {
    bird->velocity += bird->acceleration * dt;
    if (bird->velocity > 350.0f) bird->velocity = 350.0f;

    bird->position.y += bird->velocity * dt;

    if (bird->position.y < 0.0f) {
        bird->position.y = 0.0f;
        bird->velocity = 0.0f;
    }
}

void BirdFlap(Bird *bird) {
    bird->velocity = FLAP_STRENGTH;
}

bool BirdHitWorld(const Bird *bird) {
    float halfH = bird->size.y * 0.5f;
    if (bird->position.y + halfH >= SCREEN_HEIGHT) return true;
    if (bird->position.y - halfH <= 0.0f) return true;
    return false;
}

Rectangle BirdGetRect(const Bird *bird) {
    return (Rectangle){
        bird->position.x - bird->size.x * 0.5f,
        bird->position.y - bird->size.y * 0.5f,
        bird->size.x,
        bird->size.y
    };
}

void DrawBirdSprite(const Game *game) {
    const Bird *bird = &game->bird;

    // Use full 27x20 bird.png
    Rectangle src = {
        0.0f,
        0.0f,
        (float)game->texBird.width,
        (float)game->texBird.height
    };

    Rectangle dst = {
        bird->position.x,
        bird->position.y,
        bird->size.x,
        bird->size.y
    };

    Vector2 origin = { bird->size.x * 0.5f, bird->size.y * 0.5f };

    float angle = bird->velocity * 0.06f;
    if (angle > 30.0f) angle = 30.0f;
    if (angle < -30.0f) angle = -30.0f;

    DrawTexturePro(game->texBird, src, dst, origin, angle, WHITE);
}