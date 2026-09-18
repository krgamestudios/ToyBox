#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

//flags for marking areas on the terrain
#define TERRAIN_FLAG_TOTAL 32
typedef struct TerrainFlag {
	unsigned int x;
	unsigned int y;
} TerrainFlag;

//the terrain object proper
typedef struct Terrain {
	OpaqueType type;
	unsigned int width;
	unsigned int height;
	unsigned int flagMask; //signals what flags have been used, not persistent
	TerrainFlag flags[TERRAIN_FLAG_TOTAL];
	unsigned int data[];
} Terrain;

void initTerrainAPI(Toy_VM* vm);
void initTerrainReadOnlyAPI(Toy_VM* vm);

Toy_Value handleTerrainAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);

//utils for managing the terrain from C
Terrain* getTerrainPtr();
unsigned int getTerrainTile(Terrain* terrain, int x, int y);
TerrainFlag getTerrainFlag(Terrain* terrain, unsigned int index);