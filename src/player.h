#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

// --- BIRD CONSTANTS ---
#define GRAVITY        1000.0f
#define FLAP_STRENGTH  -350.0f
#define BIRD_WIDTH     34
#define BIRD_HEIGHT    24

// Forward declaration of Game (for DrawBirdSprite signature)
typedef struct Game Game;

// --- BIRD STRUCTURE ---
typedef struct Bird {
    Vector2 position;
    Vector2 size;
    float velocity;
    float acceleration;
    float rotation;
    int frame;
    float frameTimer;
} Bird;

// --- FUNCTION PROTOTYPES ---
void InitBird(Bird *bird);
void UpdateBird(Bird *bird, float dt);
void BirdFlap(Bird *bird);
bool BirdHitWorld(const Bird *bird);
Rectangle BirdGetRect(const Bird *bird);
void DrawBirdSprite(const Game *game);

#endif // PLAYER_H