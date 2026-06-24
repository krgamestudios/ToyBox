#include "toy_console_colors.h"

#include "raylib.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_vm.h"
#include "toy_attributes.h"

#include "keyboard.h"
#include "mouse.h"
#include "camera.h"
#include "actor.h"
#include "tile_grid.h"

#include "standard_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//utils
static int errorAndContinueCallback(const char* msg) {
	return fprintf(stderr, TOY_CC_ERROR "Error: %s" TOY_CC_RESET "\n", msg);
}

static int assertFailureAndExitCallback(const char* msg) {
	fprintf(stderr, TOY_CC_ASSERT "Assert Failure: %s" TOY_CC_RESET "\n", msg);
	exit(-1);
}

unsigned char* readFile(const char* path, int* size) {
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

	buffer[(*size)] = '\0';

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

//command line arguments
typedef struct Settings {
	bool error;
	bool help;
	bool version;
	const char* script;
	bool verbose; //TODO: verbose info missing
	bool uncappedFPS;
} Settings;

void usageInfo(int argc, const char* argv[]) {
	(void)argc;
	printf("Usage: %s [-h] [-v] [-c] [-f script.toy]\n\n", argv[0]);
}

void helpInfo(int argc, const char* argv[]) {
	usageInfo(argc, argv);

	printf("ToyBox, a game engine for The Toy Programming Langauge. Requires raylib and Toy by default.\nSee https://toylang.com for details.\n\n");

	printf("  -h, --help\t\t\tShow this help then exit.\n");
	printf("  -v, --version\t\t\tShow version and copyright information then exit.\n");
	printf("  -f, --file script\t\tStart the game with the given script.\n");
	// printf("  -d, --verbose\t\tPrint debugging information about Toy's internals.\n");
	printf("  -c, --no-fps-cap\t\tRemove the 60 FPS cap.\n");

	printf("\n");
}

void versionInfo(int argc, const char* argv[]) {
	(void)argc;
	(void)argv;

	const char* boxLicense =
		"Copyright (c) 2026 Kayne Ruse, KR Game Studios\n"
		"\n"
		"This software is provided 'as-is', without any express or implied\n"
		"warranty. In no event will the authors be held liable for any damages\n"
		"arising from the use of this software.\n"
		"\n"
		"Permission is granted to anyone to use this software for any purpose,\n"
		"including commercial applications, and to alter it and redistribute it\n"
		"freely, subject to the following restrictions:\n"
		"\n"
		"1. The origin of this software must not be misrepresented; you must not\n"
		"claim that you wrote the original software. If you use this software\n"
		"in a product, an acknowledgment in the product documentation would be\n"
		"appreciated but is not required.\n"
		"2. Altered source versions must be plainly marked as such, and must not be\n"
		"misrepresented as being the original software.\n"
		"3. This notice may not be removed or altered from any source distribution.\n"
		"\n"
	;

	//Toy copyright
	const char* toyLicense =
		"Copyright (c) 2020-2026 Kayne Ruse, KR Game Studios\n"
		"\n"
		"This software is provided 'as-is', without any express or implied\n"
		"warranty. In no event will the authors be held liable for any damages\n"
		"arising from the use of this software.\n"
		"\n"
		"Permission is granted to anyone to use this software for any purpose,\n"
		"including commercial applications, and to alter it and redistribute it\n"
		"freely, subject to the following restrictions:\n"
		"\n"
		"1. The origin of this software must not be misrepresented; you must not\n"
		"claim that you wrote the original software. If you use this software\n"
		"in a product, an acknowledgment in the product documentation would be\n"
		"appreciated but is not required.\n"
		"2. Altered source versions must be plainly marked as such, and must not be\n"
		"misrepresented as being the original software.\n"
		"3. This notice may not be removed or altered from any source distribution.\n"
		"\n"
	;

	printf("The ToyBox Game Engine and The Toy Programming Langauge are released under the zlib license.\n\n");

	printf("-------------------------\n\n");

	printf("The ToyBox Game Engine, This Version is in Early Development\n\n%s", boxLicense);

	printf("-------------------------\n\n");

	printf("The Toy Programming Language, Version %d.%d.%d %s\n\n%s", TOY_VERSION_MAJOR, TOY_VERSION_MINOR, TOY_VERSION_PATCH, TOY_VERSION_BUILD, toyLicense);
}

Settings parseSettings(int argc, const char* argv[]) {
	Settings settings = {0};

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			settings.help = true;
		}

		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			settings.version = true;
		}

		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file")) {
			if (argc <= i + 1) {
				settings.error = true;
				break;
			}

			if (settings.script != NULL) {
				fprintf(stderr, TOY_CC_ERROR "ERROR: No more than one script file allowed\n" TOY_CC_RESET);
				settings.error = true;
				break;
			}

			i++;
			settings.script = argv[i];
		}

		else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--verbose")) {
			settings.verbose = true;
		}

		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--no-fps-cap")) {
			settings.uncappedFPS = true;
		}

		else {
			settings.error = true;
		}
	}

	return settings;
}

//static pointers
static Toy_Function* onReady = NULL;
static Toy_Function* onFrame = NULL;
static Toy_Function* onClose = NULL;
static bool uncappedFPS = false;

//game API definitions
void api_initScreen(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value caption = Toy_popStack(&vm->stack);
	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_STRING(caption) || TOY_VALUE_AS_STRING(caption)->info.type != TOY_STRING_LEAF || !TOY_VALUE_IS_INTEGER(height) || !TOY_VALUE_IS_INTEGER(width)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initScreen', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	//setup raylib
	InitWindow(TOY_VALUE_AS_INTEGER(width), TOY_VALUE_AS_INTEGER(height), TOY_VALUE_AS_STRING(caption)->leaf.data);
	if (!uncappedFPS) {
		SetTargetFPS(60);
	}

	if (!IsWindowReady()) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: raylib failed to init the window, exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	Toy_freeValue(width);
	Toy_freeValue(height);
	Toy_freeValue(caption);
}

void api_initLoop(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value valueOnClose = Toy_popStack(&vm->stack);
	Toy_Value valueOnFrame = Toy_popStack(&vm->stack);
	Toy_Value valueOnReady = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_FUNCTION(valueOnClose) && !TOY_VALUE_IS_NULL(valueOnClose)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'initLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (!TOY_VALUE_IS_FUNCTION(valueOnFrame) && !TOY_VALUE_IS_NULL(valueOnFrame)) {
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
	if (TOY_VALUE_IS_FUNCTION(valueOnFrame)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnFrame)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'initLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onFrame = TOY_VALUE_AS_FUNCTION(valueOnFrame);
	}
	if (TOY_VALUE_IS_FUNCTION(valueOnClose)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnClose)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'initLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onClose = TOY_VALUE_AS_FUNCTION(valueOnClose);
	}
}

//API opaque dispatch
Toy_Value dispatchOpaqueAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//check for correct types
	if (!TOY_VALUE_IS_OPAQUE(compound) || !TOY_VALUE_IS_STRING(attribute) || TOY_VALUE_AS_STRING(attribute)->info.type != TOY_STRING_LEAF) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameters found in 'handleOpaqueAttributes'" TOY_CC_RESET "\n");
		return TOY_VALUE_FROM_NULL(); //do not free the params here
	}

	//assume the first byte is the type
	OpaqueType* type = (OpaqueType*)TOY_VALUE_AS_OPAQUE(compound);

	switch(*type) {
		case OPAQUE_KEYBOARD:
		case OPAQUE_KEY_PRESSED:
		case OPAQUE_KEY_RELEASED:
			return handleKeyboardAttributes(vm, compound, attribute);

		case OPAQUE_MOUSE:
		case OPAQUE_MOUSE_PRESSED:
		case OPAQUE_MOUSE_RELEASED:
			return handleMouseAttributes(vm, compound, attribute);

		case OPAQUE_CAMERA:
			return handleCameraAttributes(vm, compound, attribute);

		case OPAQUE_SPRITE_DATA:
			return handleSpriteAttributes(vm, compound, attribute);

		case OPAQUE_ACTOR_DATA:
			return handleActorAttributes(vm, compound, attribute);

		case OPAQUE_TILE_SET:
			return handleTileSetAttributes(vm, compound, attribute);

		case OPAQUE_TILE_GRID:
			return handleTileGridAttributes(vm, compound, attribute);
	}

	//only reached on error
	fprintf(stderr, TOY_CC_ERROR "ERROR: Bad opaque type found in 'handleOpaqueAttributes'" TOY_CC_RESET "\n");
	return TOY_VALUE_FROM_NULL(); //do not free the params here
}

//API tools
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"InitScreen", api_initScreen},
	{"InitLoop", api_initLoop},
	{NULL, NULL},
};

//quick and dirty template for each opaque object
#define DECLARE_OPAQUE(NAME, DATAPTR, SCOPE, BUCKETHANDLE) { \
	Toy_String* name = Toy_toString(BUCKETHANDLE, NAME); \
	Toy_declareScope(SCOPE, name, TOY_VALUE_OPAQUE, TOY_OPAQUE_FROM_POINTER(DATAPTR), true); \
	Toy_freeString(name); \
}

void initGameAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize game API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	Toy_setOpaqueAttributeHandler(dispatchOpaqueAttributes);

	//declare each function in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);
		Toy_declareScope(vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);
		Toy_freeString(key);
	}

	DECLARE_OPAQUE("Keyboard", &keyboardData, vm->scope, &vm->memoryBucket);
	DECLARE_OPAQUE("KeyPressed", &keyPressedData, vm->scope, &vm->memoryBucket);
	DECLARE_OPAQUE("KeyReleased", &keyReleasedData, vm->scope, &vm->memoryBucket);

	DECLARE_OPAQUE("Mouse", &mouseData, vm->scope, &vm->memoryBucket);
	DECLARE_OPAQUE("MousePressed", &mousePressedData, vm->scope, &vm->memoryBucket);
	DECLARE_OPAQUE("MouseReleased", &mouseReleasedData, vm->scope, &vm->memoryBucket);

	DECLARE_OPAQUE("Camera", &cameraData, vm->scope, &vm->memoryBucket);

	//opaque APIs
	initActorAPI(vm);
	initTileGridAPI(vm);
}

//main file
int main(int argc, const char* argv[]) {
	//not necessary, but nice to have
	Toy_setPrintCallback(puts);
	Toy_setErrorCallback(errorAndContinueCallback);
	Toy_setAssertFailureCallback(assertFailureAndExitCallback);

	//read settings and handle errors
	Settings settings = parseSettings(argc, argv);
	if (settings.error) {
		usageInfo(argc, argv);
		return 1;
	}
	else if (settings.help) {
		helpInfo(argc, argv);
		return 0;
	}
	else if (settings.version) {
		versionInfo(argc, argv);
		return 0;
	}
	uncappedFPS = settings.uncappedFPS; //static, because lazy

	//load the entry point
	int size = 0;
	const char* source = (char*)readFile(settings.script != NULL ? settings.script : "assets/main.toy", &size);

	if (!source) {
		fprintf(stderr, TOY_CC_ERROR "File read error: " TOY_CC_RESET);
		if (size == -1) {
			fprintf(stderr, TOY_CC_ERROR "Couldn't find '%s'\n" TOY_CC_RESET, settings.script != NULL ? settings.script : "assets/main.toy");
		}
		else {
			fprintf(stderr, TOY_CC_ERROR "Cause unknown\n" TOY_CC_RESET);
		}
		return -1;
	}

	unsigned char* entryCode = makeCodeFromSource(source);

	//build and run the VM with APIs
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, entryCode, NULL);

	initStandardLibrary(&vm);
	initGameAPI(&vm);

	Toy_runVM(&vm);
	Toy_resetVM(&vm, false, false); //leave in a valid, but unset state

	//setup and run the given loop functions
	if (onReady != NULL) {
		Toy_bindVM(&vm, onReady->bytecode.code, onReady->bytecode.parentScope);
		Toy_runVM(&vm);
		Toy_resetVM(&vm, false, false);
	}

	//onFrame is called each frame
	if (onFrame != NULL) {
		Toy_bindVM(&vm, onFrame->bytecode.code, onFrame->bytecode.parentScope);
	}

	while (!WindowShouldClose()) {
		//process the actors (if possible)
		processActors(&vm);

		//begin drawing before the call to onFrame
		BeginDrawing();
		ClearBackground(RAYWHITE);

		//run the onFrame function
		Toy_runVM(&vm); //no check needed, empty VMs are skipped

		drawActors(&vm);
		DrawFPS(0,0);
		EndDrawing();
	}

	//clear after the final frame
	if (onFrame != NULL) {
		Toy_resetVM(&vm, false, false);
	}

	//cleanup
	if (onClose != NULL) {
		Toy_bindVM(&vm, onClose->bytecode.code, onClose->bytecode.parentScope);
		Toy_runVM(&vm);
		Toy_resetVM(&vm, false, false);
	}

	freeActorAPI(&vm);

	Toy_freeVM(&vm);
	free(entryCode);

	if (IsWindowReady()) {
		CloseWindow();
	}

	return 0;
}
