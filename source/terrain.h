#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

typedef struct Terrain {
	OpaqueType type;
	unsigned int width;
	unsigned int height;
	unsigned int data[];
} Terrain;

void initTerrainAPI(Toy_VM* vm);
void initTerrainReadOnlyAPI(Toy_VM* vm);

Toy_Value handleTerrainAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);

Terrain* getTerrainPtr();