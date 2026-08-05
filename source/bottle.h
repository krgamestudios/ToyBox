#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//bottles are pure data acted upon by player scripts
typedef struct BottleData {
	OpaqueType type;
	Vector2 position;
} BottleData;

//opaque API
void initBottleAPI(Toy_VM* vm);
void freeBottleAPI(Toy_VM* vm);

//opaque binding
Toy_Value handleBottleAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);