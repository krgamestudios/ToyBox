#include "toy_console_colors.h"

#include "raylib.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_vm.h"

#include "monster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//utils
unsigned char* readFile(char* path, int* size) {
	//open the file
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		*size = -1; //missing file error
		return NULL;
	}

	//determine the file's length
	fseek(file, 0L, SEEK_END);
	*size = ftell(file);
	rewind(file);

	//make some space
	unsigned char* buffer = malloc(*size + 1);
	if (buffer == NULL) {
		fclose(file);
		return NULL;
	}

	//read the file
	if (fread(buffer, sizeof(unsigned char), *size, file) < (unsigned int)(*size)) {
		fclose(file);
		free(buffer);
		*size = -2; //singal a read error
		return NULL;
	}

	buffer[(*size)++] = '\0';

	//clean up and return
	fclose(file);
	return buffer;
}

unsigned char* makeCodeFromSource(const char* source) {
	Toy_Lexer lexer;
	Toy_bindLexer(&lexer, source);

	Toy_Parser parser;
	Toy_bindParser(&parser, &lexer);

	Toy_Bucket* bucket = Toy_allocateBucket(TOY_BUCKET_IDEAL);

	Toy_Ast* ast = Toy_scanParser(&bucket, &parser);
	unsigned char* code = Toy_compileToBytecode(ast);

	Toy_freeBucket(&bucket);

	return code;
}

//player data
typedef struct PlayerData {
	Texture2D texture;
	Rectangle rect;
	Vector2 position;
	//TODO: hitbox, hurtbox, motion, etc.
} PlayerData;

PlayerData loadPlayerData(const char* fileName, Rectangle rect) {
	PlayerData player = {0};
	player.texture = LoadTexture(fileName);
	player.rect = rect;
	return player;
}

void unloadPlayerData(PlayerData player) {
	UnloadTexture(player.texture);
}

//main file
int main() {
	//example Toy controlling the window stuff
	int size = 0;
	const char* source = (char*)readFile("assets/main.toy", &size);

	if (!source) {
		fprintf(stderr, "File read error: %d\n", size);
		return -1;
	}

	unsigned char* configCode = makeCodeFromSource(source);
	unsigned char* invokeOnReady = makeCodeFromSource("onReady();");
	unsigned char* invokeOnStep = makeCodeFromSource("onStep();");
	unsigned char* invokeOnFinished = makeCodeFromSource("onFinished();");

	//build and run the VM
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, configCode, NULL);
	Toy_runVM(&vm);
	Toy_resetVM(&vm, true, false); //leave it in a valid, but unset state

	//extract the settings
	Toy_Value* screenWidthPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenWidth") );
	Toy_Value* screenHeightPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenHeight") );
	Toy_Value* screenCaptionPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenCaption") );

	int screenWidth = screenWidthPtr != NULL && TOY_VALUE_IS_INTEGER(*screenWidthPtr) ? TOY_VALUE_AS_INTEGER(*screenWidthPtr) : 640;
	int screenHeight = screenHeightPtr != NULL && TOY_VALUE_IS_INTEGER(*screenHeightPtr) ? TOY_VALUE_AS_INTEGER(*screenHeightPtr) : 480;
	const char* screenCaption = screenCaptionPtr != NULL && TOY_VALUE_IS_STRING(*screenCaptionPtr) ? TOY_VALUE_AS_STRING(*screenCaptionPtr)->leaf.data : "";

	//setup raylib
	InitWindow(screenWidth, screenHeight, screenCaption);
	SetTargetFPS(60);

	//load a sprite
	PlayerData player = loadPlayerData("assets/parvati.png", (Rectangle){0,0,32,32});

	//initialize the monster object pool and run the setup function
	initMonsterObjectPool(&vm);

	//setup
	Toy_bindVM(&vm, invokeOnReady, NULL);
	Toy_runVM(&vm);
	Toy_resetVM(&vm, true, false);

	//onStep is called each frame
	Toy_bindVM(&vm, invokeOnStep, NULL);

	while (!WindowShouldClose()) {
		//input
		if (IsKeyDown(KEY_UP)) player.position.y -= 5.0f;
		if (IsKeyDown(KEY_DOWN)) player.position.y += 5.0f;
		if (IsKeyDown(KEY_LEFT)) player.position.x -= 5.0f;
		if (IsKeyDown(KEY_RIGHT)) player.position.x += 5.0f;

		//run the onStep function
		Toy_runVM(&vm);

		//drawing
		BeginDrawing();
		ClearBackground(RAYWHITE);

		//draw the player
		DrawTextureRec(player.texture, player.rect, player.position, WHITE);

		drawMonsters(&vm);

		DrawFPS(0,0);
		EndDrawing();
	}

	//clear onStep
	Toy_resetVM(&vm, true, false);

	//cleanup
	Toy_bindVM(&vm, invokeOnFinished, NULL);
	Toy_runVM(&vm);
	Toy_resetVM(&vm, true, false);

	freeMonsterObjectPool(&vm);
	unloadPlayerData(player);

	CloseWindow();

	Toy_freeVM(&vm);
	free(invokeOnReady);
	free(invokeOnStep);
	free(invokeOnFinished);
	free(configCode);

	return 0;
}
