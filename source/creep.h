#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

typedef struct Creep {
	OpaqueType type;
	bool active;
	Vector2 position;
} Creep;

Toy_Value handleCreepAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);