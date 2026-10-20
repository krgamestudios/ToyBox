#include "toy_console_colors.h"

#include "raylib.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_vm.h"

#include "standard_library.h"
#include "keyboard.h"
#include "mouse.h"
#include "direction.h"
#include "database.h"

#include "ast_inspector.h"
#include "bucket_inspector.h"
#include "bytecode_inspector.h"
#include "scope_inspector.h"
#include "stack_inspector.h"

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

static void raylibLoggerCallback(int msgType, const char *text, va_list args) {
	if (msgType >= LOG_ERROR) {
		fprintf(stderr, TOY_CC_ERROR);
	}
	else if (msgType >= LOG_WARNING) {
		fprintf(stderr, TOY_CC_WARN);
	}

	vfprintf(stderr, text, args);
	fprintf(stderr, TOY_CC_RESET "\n");
}

static void sqliteErrorAndContinueCallback(void* ptr, int err, const char* msg) {
	(void)ptr;
	fprintf(stderr, TOY_CC_ERROR "SQL Error Code %d: %s" TOY_CC_RESET "\n", err, msg);
}

//command line arguments
typedef struct Settings {
	bool error;
	bool help;
	bool uncappedFPS;
	unsigned char verbosity; //0 = None, 1 = simple, 2 = extra, 3 = all, 255 = suppress everything
} Settings;

void usageInfo(int argc, const char* argv[]) {
	(void)argc;
	printf("Usage: %s [-h] [-c] [-d]\n\n", argv[0]);
}

void helpInfo(int argc, const char* argv[]) {
	usageInfo(argc, argv);

	printf("The ToyBox engine, a WIP game engine.\n\n");

	printf("  -h,\t--help\t\t\tShow this help then exit.\n");
	printf("  -c,\t--no-fps-cap\tDo not limit the FPS to a maximum value (this might break some logic).\n");
	printf("  -d,\t--verbose\t\tPrint debugging information.\n");
	printf("  -dd,\t--verbose-extra\t\tN/a\n");
	printf("  -ddd,\t--verbose-all\t\tN/a\n");
	printf("  -q,\t--quiet\t\t\tN/a\n");
}

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

Settings parseSettings(int argc, const char* argv[]) {
	Settings settings = {0};

	for (int i = 1; i < argc; i++) {
		if (CSTR_MATCH(argv[i], "-h") || CSTR_MATCH(argv[i], "--help")) {
			settings.help = true;
		}

		else if (CSTR_MATCH(argv[i], "-c") || CSTR_MATCH(argv[i], "--no-fps-cap")) {
			settings.uncappedFPS = true;
		}

		else if (CSTR_MATCH(argv[i], "-d") || CSTR_MATCH(argv[i], "--verbose")) {
			settings.verbosity = 1;
		}

		else if (CSTR_MATCH(argv[i], "-dd") || CSTR_MATCH(argv[i], "--verbose-extra")) {
			settings.verbosity = 2;
		}

		else if (CSTR_MATCH(argv[i], "-ddd") || CSTR_MATCH(argv[i], "--verbose-all")) {
			settings.verbosity = 3;
		}

		else if (CSTR_MATCH(argv[i], "-q") || CSTR_MATCH(argv[i], "--quiet")) {
			settings.verbosity = 255;
		}

		else {
			settings.error = true;
		}
	}

	return settings;
}

//static globals
static bool uncappedFPS = false;
static unsigned char verbosity = false;

static Toy_Function* onReady = NULL;
static Toy_Function* onFrame = NULL;
static Toy_Function* onClose = NULL;

//extern definitions
sqlite3* database = NULL;

//engine API definitions
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
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'InitScreen', exiting" TOY_CC_RESET "\n");
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
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'InitLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (!TOY_VALUE_IS_FUNCTION(valueOnFrame) && !TOY_VALUE_IS_NULL(valueOnFrame)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'InitLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (!TOY_VALUE_IS_FUNCTION(valueOnReady) && !TOY_VALUE_IS_NULL(valueOnReady)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'InitLoop', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (TOY_VALUE_IS_FUNCTION(valueOnReady)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnReady)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'InitLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onReady = TOY_VALUE_AS_FUNCTION(valueOnReady);
	}
	if (TOY_VALUE_IS_FUNCTION(valueOnFrame)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnFrame)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'InitLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		onFrame = TOY_VALUE_AS_FUNCTION(valueOnFrame);
	}
	if (TOY_VALUE_IS_FUNCTION(valueOnClose)) {
		if (TOY_VALUE_AS_FUNCTION(valueOnClose)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'InitLoop', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
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
		case OPAQUE_KEYBOARD_PRESSED:
		case OPAQUE_KEYBOARD_RELEASED:
			return handleKeyboardAttributes(vm, compound, attribute);

		case OPAQUE_MOUSE:
		case OPAQUE_MOUSE_PRESSED:
		case OPAQUE_MOUSE_RELEASED:
			return handleMouseAttributes(vm, compound, attribute);

		case OPAQUE_DIRECTION:
			return handleDirectionAttributes(vm, compound, attribute);
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
#define DECLARE_OPAQUE(BUCKETHANDLE, SCOPE, NAME, DATAPTR) { \
	Toy_String* name = Toy_toString(BUCKETHANDLE, NAME); \
	Toy_declareScope(BUCKETHANDLE, SCOPE, name, TOY_VALUE_OPAQUE, TOY_OPAQUE_FROM_POINTER(DATAPTR), true); \
	Toy_freeString(name); \
}

void initEngineAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize game API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each function in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);
		Toy_declareScope(&vm->memoryBucket, vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);
		Toy_freeString(key);
	}

	Toy_setOpaqueAttributeHandler(dispatchOpaqueAttributes);

	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Keyboard", &keyboardData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "KeyPressed", &keyPressedData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "KeyReleased", &keyReleasedData);

	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Mouse", &mouseData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "MousePressed", &mousePressedData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "MouseReleased", &mouseReleasedData);

	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Direction", &directionData);
}

//main file
int main(int argc, const char* argv[]) {
	//if there's args, process them
	Settings settings = parseSettings(argc, argv);
	if (settings.error) {
		usageInfo(argc, argv);
		return 1;
	}

	//configure the output options
	Toy_setPrintCallback(puts);
	Toy_setErrorCallback(errorAndContinueCallback);
	Toy_setAssertFailureCallback(assertFailureAndExitCallback);
	SetTraceLogCallback(raylibLoggerCallback);
	sqlite3_config(SQLITE_CONFIG_LOG, sqliteErrorAndContinueCallback);

	if (settings.verbosity == 255) {
		SetTraceLogLevel(LOG_ERROR);
	}
	else if (settings.verbosity >= 1) {
		SetTraceLogLevel(LOG_INFO);
	}
	else {
		SetTraceLogLevel(LOG_WARNING);
	}

	if (settings.help) {
		helpInfo(argc, argv);
		return 0;
	}

	//static flags to avoid extra params
	verbosity = settings.verbosity;
	uncappedFPS = settings.uncappedFPS;

	//open the database
	sqlite3_open("save.db", &database);

	//run the setup script
	int size = 0;
	const char* setupFile = "assets/setup.toy";
	const char* source = (char*)Toy_private_readFile(setupFile, &size);

	if (!source) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Failed to read setup script, " TOY_CC_RESET);
		if (size == -1) {
			fprintf(stderr, TOY_CC_ERROR "Couldn't find %s\n" TOY_CC_RESET, setupFile);
		}
		else {
			fprintf(stderr, TOY_CC_ERROR "Cause unknown\n" TOY_CC_RESET);
		}
		return -1;
	}

	char workingDir[256];
	Toy_private_getWorkingDir(workingDir, setupFile, 256);
	unsigned char* setupBytecode = Toy_private_makeCodeFromSource(source, workingDir);

	//build and run the VM with all the APIs
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, setupBytecode, NULL);

	//initialize the libraries
	initStandardLibrary(&vm);
	initEngineAPI(&vm);

	if (verbosity >= 1 && verbosity < 100) {
		inspect_bytecode(vm.code);
	}

	Toy_runVM(&vm);
	Toy_resetVM(&vm, true, false); //leave in a valid state, preserving the root scope

	//setup and run the given loop functions, if able
	if (onReady != NULL) {
		Toy_bindVM(&vm, onReady->bytecode.code, onReady->bytecode.parentScope);
		Toy_runVM(&vm);
		Toy_resetVM(&vm, true, false);
	}

	//onFrame is called each frame
	if (onFrame != NULL) {
		Toy_bindVM(&vm, onFrame->bytecode.code, onFrame->bytecode.parentScope);
	}

	while (!WindowShouldClose()) {
		//run the onFrame function
		Toy_runVM(&vm); //no check needed, empty VMs are skipped

		//TODO: process game logic

		//rendering all at once
		BeginDrawing();
		ClearBackground(WHITE); //TODO: remove this once the whole screen is covered

		if (verbosity >= 1 && verbosity < 100) {
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
	free(setupBytecode);

	if (IsWindowReady()) {
		CloseWindow();
	}

	sqlite3_close(database);

	return 0;
}
