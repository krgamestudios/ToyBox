#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

typedef struct TileGridData {
	OpaqueType type;
} TileGridData;

extern TileGridData tileGridData;

void api_initTileGrid(Toy_VM* vm, Toy_FunctionNative* self);

Toy_Value handleTileGridAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
