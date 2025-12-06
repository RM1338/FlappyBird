#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "game.h"

void InitBird(Bird *bird);
void UpdateBird(Bird *bird, float dt);
void BirdFlap(Bird *bird);
bool BirdHitWorld(const Bird *bird);
Rectangle BirdGetRect(const Bird *bird);
void DrawBirdSprite(const Game *game);

#endif