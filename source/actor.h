#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "toy_table.h"
#include "toy_function.h"
#include "raylib.h"

//WARN: Sprites and Actors are tightly coupled, but that's good enough for the moment

//sprites loaded from disk, with zero or more states
typedef struct SpriteData {
	OpaqueType type;
	Texture2D texture;
	Rectangle rect;
	Toy_Table* states;
} SpriteData;

//assumes each state is a single row on the sprite sheet
typedef struct SpriteState {
	unsigned int stripIndex;
	unsigned int frameCount;
} SpriteState;

//Actors opaques loaded from scripts
typedef struct ActorData {
	OpaqueType type;

	SpriteData* spriteData;
	SpriteState* spriteState;
	unsigned int spriteFrameTick;
	unsigned int spriteFrameDuration;

	Vector2 position;

	Toy_Function* onStep;
} ActorData;

//opaque API
void initActorAPI(Toy_VM* vm);
void freeActorAPI(Toy_VM* vm);

void processActors(Toy_VM* vm);
void drawActors(Toy_VM* vm);

//opaque binding
Toy_Value handleSpriteAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);