#pragma once

#include "raylib.h"

//Supports multiple frames of animation
typedef struct Sprite {
	Texture2D texture;
	Rectangle frame;
	Vector2 frameOffset;
	unsigned int frameCount;
	unsigned int frameIndex;
	unsigned int delayCount;
	unsigned int delayIndex;
} Sprite;

Sprite makeSpriteFromTexture(Texture2D texture);
Sprite makeSpriteFromTextureRect(Texture2D texture, Rectangle frame);
Sprite makeSpriteFromTextureAnim(Texture2D texture, Rectangle frame, Vector2 frameOffset, unsigned int frameCount, unsigned int delay);

void drawSpriteTo(Sprite* sprite, Vector2 position);
