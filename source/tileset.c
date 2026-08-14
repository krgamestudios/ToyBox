#include "tileset.h"

Tileset loadTileset(const char* filename, unsigned int tileWidth, unsigned int tileHeight) {
	return (Tileset){
		.texture = LoadTexture(filename),
		.tileWidth = tileWidth,
		.tileHeight = tileHeight
	};
}

void unloadTileset(Tileset tileset) {
	UnloadTexture(tileset.texture);
}

void drawDataWithTileset(Tileset tileset, unsigned int width, unsigned int height, unsigned int data[]) {
	//how many tiles in tileset
	unsigned int xcount = tileset.texture.width / tileset.tileWidth;
	unsigned int ycount = tileset.texture.height / tileset.tileHeight;

	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			//tile to draw
			unsigned int tile = data[j * width + i];

			DrawTexturePro(
				tileset.texture,
				//source
				(Rectangle){
					(tile % xcount) * tileset.tileWidth,
					(tile / ycount) * tileset.tileHeight,
					tileset.tileWidth,
					tileset.tileHeight,
				},
				//dest
				(Rectangle){
					i * tileset.tileWidth,
					j * tileset.tileHeight,
					tileset.tileWidth,
					tileset.tileHeight,
				},
				(Vector2){0,0}, //origin
				0, //rotation
				WHITE //tint
			);
		}
	}
}