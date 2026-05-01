#pragma once

#include "raylib.h"

//sprites loaded from disk
typedef struct MonsterSprite {
	Texture2D texture;
	Rectangle rect;
} MonsterSprite;

//Monsters loaded from scripts
typedef struct MonsterData {
	MonsterSprite* sprite;
	Vector2 position;
	int health;
} MonsterData;

