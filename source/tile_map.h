#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

typedef struct TileSetData {
	OpaqueType type;
	Texture2D texture;
	unsigned int tileWidth;
	unsigned int tileHeight;
} TileSetData;

typedef struct TileMapData {
	OpaqueType type;
	unsigned int width;
	unsigned int height;
	unsigned int *cells;
} TileMapData;

void initTileMapAPI(Toy_VM* vm);
// void freeTileMapAPI(Toy_VM* vm);

Toy_Value handleTileSetAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
Toy_Value handleTileMapAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
