#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

typedef struct Core {
	OpaqueType type;
	Vector2 position;
} Core;

Toy_Value handleCoreAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);