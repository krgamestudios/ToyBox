#pragma once

#include "raylib.h"

typedef struct Tileset {
	Texture2D texture;
	unsigned int tileWidth;
	unsigned int tileHeight;
} Tileset;

Tileset loadTileset(const char* filename, unsigned int tileWidth, unsigned int tileHeight);
void unloadTileset(Tileset tileset);

void drawDataWithTileset(Tileset tileset, unsigned int width, unsigned int height, unsigned int data[]);
