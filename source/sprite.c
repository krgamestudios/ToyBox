#include "sprite.h"

Sprite makeSpriteFromTexture(Texture2D texture) {
	return makeSpriteFromTextureRect(texture, (Rectangle){0, 0, texture.width, texture.height});
}

Sprite makeSpriteFromTextureRect(Texture2D texture, Rectangle frame) {
	Sprite sprite = {
		.texture = texture,
		.frame = frame,
		.frameOffset = {0,0},
		.frameCount = 0,
		.frameIndex = 0,
		.delayCount = 0,
		.delayIndex = 0,
	};

	return sprite;
}

Sprite makeSpriteFromTextureAnim(Texture2D texture, Rectangle frame, Vector2 frameOffset, unsigned int frameCount, unsigned int delay) {
	Sprite sprite = {
		.texture = texture,
		.frame = frame,
		.frameOffset = frameOffset,
		.frameCount = frameCount,
		.frameIndex = 0,
		.delayCount = delay,
		.delayIndex = 0,
	};

	return sprite;
}

void drawSpriteTo(Sprite* sprite, Vector2 position) {
	if (sprite->frameCount > 0) {
		Rectangle rect = {
			.x = sprite->frame.x + sprite->frameOffset.x * sprite->frameIndex,
			.y = sprite->frame.y + sprite->frameOffset.y * sprite->frameIndex,
			.width = sprite->frame.width,
			.height = sprite->frame.height,
		};
		DrawTextureRec(sprite->texture, rect, position, WHITE);

		//
		if (sprite->delayCount > 0) {
			if (++sprite->delayIndex >= sprite->delayCount) {
				sprite->delayIndex = 0;
				if (++sprite->frameIndex >= sprite->frameCount) { //WARN: duped below
					sprite->frameIndex = 0;
				}
			}
		}
		else if (sprite->frameCount > 0) {
			if (++sprite->frameIndex >= sprite->frameCount) {
				sprite->frameIndex = 0;
			}
		}
	}
	else {
		DrawTextureRec(sprite->texture, sprite->frame, position, WHITE);
	}
}
