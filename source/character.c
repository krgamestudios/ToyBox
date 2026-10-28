#include "character.h"

Character loadCharacter(const char* fname) {
	Texture2D tex = LoadTexture(fname);

	Character character = {
		.position = {0,0},
		.sprite = makeSpriteFromTexture(tex),
		.square = makeAASquare(0, 0, tex.width, tex.height),
	};

	return character;
}

void unloadCharacter(Character character) {
	UnloadTexture(character.sprite.texture);
}