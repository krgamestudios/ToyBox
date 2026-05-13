#pragma once

#include "toy_vm.h"
#include "toy_function.h"
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
	Toy_Function* onStep;
	Vector2 position;
	bool enabled;
} ActorData;

//object pool system
void initActorAPI(Toy_VM* vm);
void freeActorAPI(Toy_VM* vm);

void processActors(Toy_VM* vm);
void drawActors(Toy_VM* vm);

void loadSprite(Toy_Bucket** bucketHandle, Toy_Value key, const char* fname, int width, int height);
ActorData* spawnActorAt(Toy_Bucket** bucketHandle, Toy_Value key, Toy_Function* onStep, int xpos, int ypos);

//opaque hook
Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);