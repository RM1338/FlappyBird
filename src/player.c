#include "player.h"
#include "game.h"

// --- BIRD FUNCTIONS ---

void InitBird(Bird *bird) {
    bird->position = (Vector2){ 100.0f, SCREEN_HEIGHT / 2.0f };
    bird->size     = (Vector2){ BIRD_WIDTH, BIRD_HEIGHT };  // e.g. 27x20 or 34x24
    bird->velocity = 0.0f;
    bird->acceleration = GRAVITY;
    bird->rotation = 0.0f;
    bird->frame = 0;
    bird->frameTimer = 0.0f;
}

void UpdateBird(Bird *bird, float dt) {
    // Gravity
    bird->velocity += bird->acceleration * dt;
    if (bird->velocity > 350.0f) bird->velocity = 350.0f;

    // Position
    bird->position.y += bird->velocity * dt;

    // Tilt based on velocity
    bird->rotation = bird->velocity * 0.06f;
    if (bird->rotation > 70.0f)  bird->rotation = 70.0f;
    if (bird->rotation < -45.0f) bird->rotation = -45.0f;

    // Prevent going off the top
    if (bird->position.y - bird->size.y * 0.5f < 0.0f) {
        bird->position.y = bird->size.y * 0.5f;
        bird->velocity = 0.0f;
    }
}

void BirdFlap(Bird *bird) {
    bird->velocity = FLAP_STRENGTH;
    bird->rotation = -45.0f;
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

// Draw using ENTIRE bird.png as a single frame (no sprite sheet)
void DrawBirdSprite(const Game *game) {
    const Bird *bird = &game->bird;

    Rectangle sourceRec = {
        0.0f,
        0.0f,
        (float)game->texBird.width,
        (float)game->texBird.height
    };

    Rectangle destRec = {
        bird->position.x,
        bird->position.y,
        bird->size.x,
        bird->size.y
    };

    Vector2 origin = { bird->size.x * 0.5f, bird->size.y * 0.5f };
    float angle = bird->rotation;

    DrawTexturePro(
        game->texBird,
        sourceRec,
        destRec,
        origin,
        angle,
        WHITE
    );
}