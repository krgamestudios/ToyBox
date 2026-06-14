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

typedef struct TileGridData {
	OpaqueType type;
	unsigned int width;
	unsigned int height;
	unsigned int *cells;
} TileGridData;

void initTileGridAPI(Toy_VM* vm);
// void freeTileGridAPI(Toy_VM* vm);

Toy_Value handleTileSetAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
Toy_Value handleTileGridAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
