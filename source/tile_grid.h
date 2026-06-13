#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

typedef struct TileGridData {
	OpaqueType type;
	unsigned int width;
	unsigned int height;
	unsigned int *cells;
} TileGridData;

void initTileGridAPI(Toy_VM* vm);

Toy_Value handleTileGridAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
