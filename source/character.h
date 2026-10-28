#pragma once

#include "sprite.h"
#include "aasquare.h"
#include "raylib.h"

typedef struct Character {
	Vector2 position;
	Sprite sprite;
	AASquare square;
} Character;

Character loadCharacter(const char* fname);

void unloadCharacter(Character character);