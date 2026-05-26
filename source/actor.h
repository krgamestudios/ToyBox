#pragma once

#include "opaque_type.h"
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
	OpaqueType type;
	SpriteData* sprite;
	Toy_Function* onStep;
	Vector2 position;
	bool enabled;
} ActorData;

//opaque API
void initActorAPI(Toy_VM* vm);
void freeActorAPI(Toy_VM* vm);

void processActors(Toy_VM* vm);
void drawActors(Toy_VM* vm);

//opaque binding
Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);