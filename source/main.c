#include "toy_console_colors.h"

#include "raylib.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_vm.h"
#include "toy_attributes.h"

#include "keyboard.h"
#include "mouse.h"
#include "terrain.h"
#include "tileset.h"

#include "standard_library.h"
#include "scope_inspector.h"

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
	bool verbose;
	bool uncappedFPS;
} Settings;

void usageInfo(int argc, const char* argv[]) {
	(void)argc;
	printf("Usage: %s [-h] [-v] [-c] [-f script.toy]\n\n", argv[0]);
}

Settings parseSettings(int argc, const char* argv[]) {
	Settings settings = {0};

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			settings.help = true;
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

//static members
static Toy_Function* onReady = NULL;
static Toy_Function* onFrame = NULL;
static Toy_Function* onClose = NULL;
static bool verbose = false;
static bool uncappedFPS = false;

//game API definitions
void api_initScreen(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 3) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'InitScreen'");
		Toy_error(buffer);
		return;
	}

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

	if (vm->stack->count < 3) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'InitLoop'");
		Toy_error(buffer);
		return;
	}

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

		case OPAQUE_TERRAIN:
			return handleTerrainAttributes(vm, compound, attribute);
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
}

//main file
int main(int argc, const char* argv[]) {
	//not necessary, but nice to have
	Toy_setPrintCallback(puts);
	Toy_setErrorCallback(errorAndContinueCallback);
	Toy_setAssertFailureCallback(assertFailureAndExitCallback);

	//read settings and handle errors
	Settings settings = parseSettings(argc, argv);
	if (settings.error || settings.help) {
		usageInfo(argc, argv);
		return 1;
	}

	//static flags to avoid extra params
	verbose = settings.verbose;
	uncappedFPS = settings.uncappedFPS;

	//load the entry point
	int size = 0;
	const char* source = (char*)readFile("assets/setup.toy", &size);

	if (!source) {
		fprintf(stderr, TOY_CC_ERROR "File read error: " TOY_CC_RESET);
		if (size == -1) {
			fprintf(stderr, TOY_CC_ERROR "Couldn't find setup.toy\n" TOY_CC_RESET);
		}
		else {
			fprintf(stderr, TOY_CC_ERROR "Cause unknown\n" TOY_CC_RESET);
		}
		return -1;
	}

	unsigned char* entryCode = makeCodeFromSource(source);

	//build and run the VM with all the APIs
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, entryCode, NULL);

	//initialize the libraries
	initStandardLibrary(&vm);
	initGameAPI(&vm);
	initTerrainAPI(&vm);

	Toy_runVM(&vm);

	if (verbose) {
		inspect_scope(vm.scope, 0);
	}

	Toy_resetVM(&vm, false, false); //leave in a valid, but unset state

	//load graphical assets
	Tileset tileset = loadTileset("assets/terrain.png", 16, 16);

	//setup and run the given loop functions, if able
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
		//run the onFrame function
		Toy_runVM(&vm); //no check needed, empty VMs are skipped
		//TODO: process bots

		//rendering all at once
		BeginDrawing();
		ClearBackground(WHITE); //TODO: remove this once the whole screen is covered

		//draw the terrain, if able
		Terrain* terrain = getTerrainPtr();
		if (terrain) {
			drawDataWithTileset(tileset, terrain->width, terrain->height, terrain->data);
		}

		//TODO: draw bots

		if (verbose) {
			DrawFPS(0,0);
		}

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

	Toy_freeVM(&vm);
	free(entryCode);

	unloadTileset(tileset);

	if (IsWindowReady()) {
		CloseWindow();
	}

	return 0;
}
