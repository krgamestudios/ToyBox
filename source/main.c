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
#include "direction.h"
#include "tileset.h"
#include "core.h"
#include "creep.h"
#include "player.h"

#include "standard_library.h"
#include "bytecode_inspector.h"

#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

//utils
static int errorAndContinueCallback(const char* msg) {
	return fprintf(stderr, TOY_CC_ERROR "Error: %s" TOY_CC_RESET "\n", msg);
}

static int assertFailureAndExitCallback(const char* msg) {
	fprintf(stderr, TOY_CC_ASSERT "Assert Failure: %s" TOY_CC_RESET "\n", msg);
	exit(-1);
}

static void sqliteErrorAndContinueCallback(void* ptr, int err, const char* msg) {
	(void)ptr;
	fprintf(stderr, TOY_CC_ERROR "SQL Error Code %d: %s" TOY_CC_RESET "\n", err, msg);
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

unsigned char* makeCodeFromSource(const char* source, const char* workingDir) {
	Toy_Lexer lexer;
	Toy_bindLexer(&lexer, source);

	Toy_Parser parser;
	Toy_bindParser(&parser, &lexer);
	Toy_adjustParserWorkingDirectory(&parser, workingDir);

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

#define PLAYERS_CAPACITY 8
static int playersCount = 0;
static Player* playersHandle[PLAYERS_CAPACITY] = {0};

//extern member definitions
sqlite3* database = NULL;

//forward declarations
Player* loadPlayer(const char* name);
void freeAllPlayers();
void tickAllPlayers();

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

void api_loadPlayer(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'LoadPlayer'");
		Toy_error(buffer);
		return;
	}

	Toy_Value nameValue = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_STRING(nameValue)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad types found in 'LoadPlayer', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	Toy_String* name = TOY_VALUE_AS_STRING(nameValue);

	if (name->info.type == TOY_STRING_LEAF) {
		playersHandle[playersCount++] = loadPlayer(name->leaf.data);
	}
	else {
		char* buffer = Toy_getStringRaw(name);
		playersHandle[playersCount++] = loadPlayer(buffer);
		free(buffer);
	}

	Toy_freeValue(nameValue);
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

		case OPAQUE_DIRECTION:
			return handleDirectionAttributes(vm, compound, attribute);

		case OPAQUE_CORE:
			return handleCoreAttributes(vm, compound, attribute);

		case OPAQUE_CREEP:
			return handleCreepAttributes(vm, compound, attribute);
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
	{"LoadPlayer", api_loadPlayer},
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

	initTerrainAPI(vm);
}

void initGameAPI(Toy_VM* vm, Player* player) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize game API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	Toy_setOpaqueAttributeHandler(dispatchOpaqueAttributes);

	//TODO: remove player inputs
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Keyboard", &keyboardData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "KeyPressed", &keyPressedData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "KeyReleased", &keyReleasedData);

	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Mouse", &mouseData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "MousePressed", &mousePressedData);
	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "MouseReleased", &mouseReleasedData);

	DECLARE_OPAQUE(&vm->memoryBucket, vm->scope, "Direction", &directionData);

	initTerrainReadOnlyAPI(vm);
	initPlayerAPI(vm, player);
}

//util for finding and loading each player
Player* loadPlayer(const char* name) {
	//build the path to the main file & working directory of the given player folder
	char mainfile[1024];
	snprintf(mainfile, 1024, "players/%s/main.toy", name);
	char workingDir[256];
	Toy_private_getWorkingDir(workingDir, mainfile, 256);

	//load & compile the player script
	int size = 0;
	const char* source = (char*)readFile(mainfile, &size);
	if (source == NULL) {
		return NULL;
	}

	unsigned char* code = makeCodeFromSource(source, workingDir);
	Player* player = allocatePlayer();
	bindBytecodeToPlayer(player, code);

	initStandardLibrary(&player->vm);
	initGameAPI(&player->vm, player);

	//place the player's core on the map
	Terrain* terrain = getTerrainPtr(); //must be called after the setup script is run
	if (terrain == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Couldn't place the player cores on non-existant terrain (The players can only be loaded after the terrain)\n" TOY_CC_RESET);
		exit(-1);
	}

	//check that not all core flags are occupied
	if ((terrain->flagMask ^ 0xFF) == 0) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Too many players allocated\n" TOY_CC_RESET);
		exit(-1);
	}

	//NOTE: first 8 flags are reserved for player core locations
	for (unsigned int i = 0; i < 8; i++) {
		if ((terrain->flagMask & (1 << i)) == 0) {
			TerrainFlag flag = getTerrainFlag(terrain, i);
			player->core.position = (Vector2){ .x = flag.x, .y = flag.y };
			terrain->flagMask |= (1 << i);
			break;
		}
	}

	return player;
}

void freeAllPlayers() {
	for (int i = 0; i < playersCount; i++) {
		freePlayer(playersHandle[i]);
	}
	playersCount = 0;
}

void tickAllPlayers() {
	for (int i = 0; i < playersCount; i++) {
		Toy_VM* pvm = &playersHandle[i]->vm;
		//this temporary scope lets players declare vars in the root of the file
		pvm->scope = Toy_pushScope(&pvm->memoryBucket, pvm->scope);
		Toy_runVM(pvm);
		pvm->scope = Toy_popScope(pvm->scope);
	}
}

//main file
int main(int argc, const char* argv[]) {
	//not necessary, but nice to have
	Toy_setPrintCallback(puts);
	Toy_setErrorCallback(errorAndContinueCallback);
	Toy_setAssertFailureCallback(assertFailureAndExitCallback);
	sqlite3_config(SQLITE_CONFIG_LOG, sqliteErrorAndContinueCallback);

	//read settings and handle errors
	Settings settings = parseSettings(argc, argv);
	if (settings.error || settings.help) {
		usageInfo(argc, argv);
		return 1;
	}

	//static flags to avoid extra params
	verbose = settings.verbose;
	uncappedFPS = settings.uncappedFPS;

	//open the database
	sqlite3_open("save.db", &database);

	//run the setup script
	int size = 0;
	const char* setupFile = "assets/setup.toy";
	const char* source = (char*)readFile(setupFile, &size);

	if (!source) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Failed to read a file, " TOY_CC_RESET);
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
	unsigned char* entryCode = makeCodeFromSource(source, workingDir);

	//build and run the VM with all the APIs
	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, entryCode, NULL);

	//initialize the libraries
	initStandardLibrary(&vm);
	initEngineAPI(&vm);

	if (verbose) {
		inspect_bytecode(vm.code);
	}

	Toy_runVM(&vm);
	Toy_resetVM(&vm, false, false); //leave in a valid, but unset state

	//load graphical assets
	Tileset tileset = loadTileset("assets/terrain.png", 16, 16);
	Texture2D coreSprite = LoadTexture("assets/Core_0.png");
	Texture2D creepSprite = LoadTexture("assets/Creep_3.png");

	if (playersCount <= 0) {
		fprintf(stderr, TOY_CC_WARN "WARNING: No players loaded" TOY_CC_RESET "\n");
	}

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

		//process player scripts
		static int ticker = 0; //TMP
		if (ticker++ % 4 == 0)
			tickAllPlayers();

		//rendering all at once
		BeginDrawing();
		ClearBackground(WHITE); //TODO: remove this once the whole screen is covered

		//draw the terrain, if able
		Terrain* terrain = getTerrainPtr();
		if (terrain) {
			drawDataWithTileset(tileset, terrain->width, terrain->height, terrain->data);
		}

		//For each player
		for (int p = 0; p < playersCount; p++) { //NOTE: positions multiplied by tile size
			//draw the cores
			DrawTexture(coreSprite,
				playersHandle[p]->core.position.x * 16,
				playersHandle[p]->core.position.y * 16,
				WHITE);

			//draw the creeps
			for (unsigned int c = 0; c < playersHandle[p]->creepCapacity; c++) {
				if (playersHandle[p]->creeps[c].active) {
					DrawTexture(creepSprite,
						playersHandle[p]->creeps[c].position.x * 16,
						playersHandle[p]->creeps[c].position.y * 16,
						WHITE);
				}
			}
		}

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

	freeAllPlayers();

	Toy_freeVM(&vm);
	free(entryCode);

	UnloadTexture(coreSprite);
	UnloadTexture(creepSprite);
	unloadTileset(tileset);

	if (IsWindowReady()) {
		CloseWindow();
	}

	sqlite3_close(database);

	return 0;
}
