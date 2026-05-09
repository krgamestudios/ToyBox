#pragma once

#include "toy_vm.h"
#include "raylib.h"

//sprites loaded from disk
typedef struct SpriteData {
	Texture2D texture;
	Rectangle rect;
	//TODO: animation
} SpriteData;

//Actors loaded from scripts
typedef struct ActorData {
	SpriteData* sprite;
	Vector2 position;
	//TODO: animation
	int health;
} ActorData;

//object pool system
void initActorAPI(Toy_VM* vm);
void freeActorAPI(Toy_VM* vm);

void processActorStep(Toy_VM* vm);
void drawActors(Toy_VM* vm);

void loadSprite(Toy_Bucket** bucketHandle, Toy_Value key, const char* fname, int width, int height);
ActorData* spawnActorAt(Toy_Bucket** bucketHandle, Toy_Value key, int xpos, int ypos);

Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);