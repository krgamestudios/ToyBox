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

//static pointers
static Toy_Function* onReady = NULL;
static Toy_Function* onStep = NULL;
static Toy_Function* onClose = NULL;

//game API definitions
void initScreen(Toy_VM* vm) {
	Toy_Value caption = Toy_popStack(&vm->stack);
	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_STRING(caption) || TOY_VALUE_AS_STRING(caption)->info.type != TOY_STRING_LEAF || !TOY_VALUE_IS_INTEGER(height) || !TOY_VALUE_IS_INTEGER(width)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initScreen', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	//setup raylib
	InitWindow(TOY_VALUE_AS_INTEGER(width), TOY_VALUE_AS_INTEGER(height), TOY_VALUE_AS_STRING(caption)->leaf.data);
	SetTargetFPS(60);

	Toy_freeValue(width);
	Toy_freeValue(height);
	Toy_freeValue(caption);
}

void initLoop(Toy_VM* vm) {
	Toy_Value valueOnClose = Toy_popStack(&vm->stack);
	Toy_Value valueOnStep = Toy_popStack(&vm->stack);
	Toy_Value valueOnReady = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_FUNCTION(valueOnClose) && !TOY_VALUE_IS_NULL(valueOnClose)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (!TOY_VALUE_IS_FUNCTION(valueOnStep) && !TOY_VALUE_IS_NULL(valueOnStep)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (!TOY_VALUE_IS_FUNCTION(valueOnReady) && !TOY_VALUE_IS_NULL(valueOnReady)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (TOY_VALUE_IS_FUNCTION(valueOnReady)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnReady)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'initLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onReady = TOY_VALUE_AS_FUNCTION(valueOnReady);
	}
	if (TOY_VALUE_IS_FUNCTION(valueOnStep)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnStep)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'initLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onStep = TOY_VALUE_AS_FUNCTION(valueOnStep);
	}
	if (TOY_VALUE_IS_FUNCTION(valueOnClose)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnClose)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'initLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onClose = TOY_VALUE_AS_FUNCTION(valueOnClose);
	}
}

//game API tools
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"initScreen", initScreen},
	{"initLoop", initLoop},
	{NULL, NULL},
};

void initGameAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize game API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each function in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_toString(&(vm->memoryBucket), callbackPairs[i].name);
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);
		Toy_declareScope(vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);
		Toy_freeString(key);
	}
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
	if (IsWindowReady()) {
		player.texture = LoadTexture(fileName);
		player.rect = rect;
	}
	return player;
}

void unloadPlayerData(PlayerData player) {
	UnloadTexture(player.texture);
}

//main file
int main() {
	//load the entry point
	int size = 0;
	const char* source = (char*)readFile("assets/main.toy", &size);

	if (!source) {
		fprintf(stderr, "File read error: %d\n", size);
		return -1;
	}

	unsigned char* entryCode = makeCodeFromSource(source);

	//build and run the VM with APIs
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, entryCode, NULL);
	initGameAPI(&vm);
	initMonsterAPI(&vm);
	Toy_runVM(&vm);
	Toy_resetVM(&vm, false, false); //leave in a valid, but unset state

	//load a sprite
	PlayerData player = loadPlayerData("assets/parvati.png", (Rectangle){0,0,32,32});

	//setup and run the given loop functions
	if (onReady != NULL) {
		Toy_bindVM(&vm, onReady->bytecode.code, onReady->bytecode.parentScope);
		Toy_runVM(&vm);
		Toy_resetVM(&vm, false, false);
	}

	//onStep is called each frame
	if (onStep != NULL) {
		Toy_bindVM(&vm, onStep->bytecode.code, onStep->bytecode.parentScope);
	}

	while (!WindowShouldClose()) {
		//input
		if (IsKeyDown(KEY_UP)) player.position.y -= 5.0f;
		if (IsKeyDown(KEY_DOWN)) player.position.y += 5.0f;
		if (IsKeyDown(KEY_LEFT)) player.position.x -= 5.0f;
		if (IsKeyDown(KEY_RIGHT)) player.position.x += 5.0f;

		//run the onStep function
		Toy_runVM(&vm); //no check needed, empty VMs are skipped

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
	if (onStep != NULL) {
		Toy_resetVM(&vm, false, false);
	}

	//cleanup
	if (onClose != NULL) {
		Toy_bindVM(&vm, onClose->bytecode.code, onClose->bytecode.parentScope);
		Toy_runVM(&vm);
		Toy_resetVM(&vm, false, false);
	}

	freeMonsterAPI(&vm);
	unloadPlayerData(player);

	if (IsWindowReady()) {
		CloseWindow();
	}

	Toy_freeVM(&vm);
	free(entryCode);

	return 0;
}
