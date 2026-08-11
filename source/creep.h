#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

typedef struct Creep {
	OpaqueType type;
	Vector2 position;
} Creep;

void initCreepAPI(Toy_VM* vm);

Toy_Value handleCreepAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);