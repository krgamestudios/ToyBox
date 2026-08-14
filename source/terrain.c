#include "terrain.h"

#include "toy_console_colors.h"

#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

//static members
static Toy_Value terrainValue = {0};
static bool terrainLocked = false;

void api_createTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
	//width, height -> Opaque(Terrain)
	(void)self;

	if (terrainLocked) {
		char buffer[256];
		snprintf(buffer, 256, "Can't call 'CreateTerrain' while terrain is locked");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CreateTerrain'");
		Toy_error(buffer);
		return;
	}

	Toy_Value width = Toy_popStack(&vm->stack);
	Toy_Value height = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'CreateTerrain'");
		Toy_error(buffer);
		Toy_freeValue(width);
		Toy_freeValue(height);
		return;
	}

	//check bounds
	if (TOY_VALUE_AS_INTEGER(width) <= 0 || TOY_VALUE_AS_INTEGER(width) > 256 || TOY_VALUE_AS_INTEGER(height) <= 0 || TOY_VALUE_AS_INTEGER(height) > 256) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter values found in 'CreateTerrain'");
		Toy_error(buffer);
		return;
	}

	//free existing terrain, if any
	if (TOY_VALUE_IS_OPAQUE(terrainValue)) {
		free(TOY_VALUE_AS_OPAQUE(terrainValue));
		Toy_freeValue(terrainValue);
	}

	Terrain* terrain = malloc(sizeof(Terrain) + sizeof(unsigned int) * TOY_VALUE_AS_INTEGER(width) * TOY_VALUE_AS_INTEGER(height));
	terrain->type = OPAQUE_TERRAIN;
	terrain->width = TOY_VALUE_AS_INTEGER(width);
	terrain->height = TOY_VALUE_AS_INTEGER(height);
	memset(terrain->data, 0, sizeof(unsigned int) * terrain->width * terrain->height);

	//leave the reference on the stack
	terrainValue = TOY_OPAQUE_FROM_POINTER(terrain);
	Toy_pushStack(&vm->stack, TOY_REFERENCE_FROM_POINTER(&terrainValue));
}

void api_unloadTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	if (terrainLocked) {
		char buffer[256];
		snprintf(buffer, 256, "Can't call 'UnloadTerrain' while terrain is locked");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'UnloadTerrain'");
		Toy_error(buffer);
		return;
	}

	Toy_Value value = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_OPAQUE(value) || ((Terrain*)TOY_VALUE_AS_OPAQUE(value))->type != OPAQUE_TERRAIN) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'UnloadTerrain'");
		Toy_error(buffer);
		Toy_freeValue(value);
		return;
	}

	Terrain* terrain = TOY_VALUE_AS_OPAQUE(value);
	free(terrain);
	if (TOY_VALUE_IS_REFERENCE(value)) {
		terrainValue = TOY_VALUE_FROM_NULL(); //temp workaround
	} //TODO: nullify scoped variable
}

void api_loadTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	if (terrainLocked) {
		char buffer[256];
		snprintf(buffer, 256, "Can't call 'LoadTerrain' while terrain is locked");
		Toy_error(buffer);
		return;
	}

	//BUGFIX: first, check if the terrain exists
	{
		sqlite3_stmt* stmt;
		sqlite3_prepare(
			database,
			"SELECT COUNT(*) FROM terrain WHERE id = ?;",
			-1,
			&stmt,
			NULL
		);
		sqlite3_bind_int(stmt, 1, 1); //id
		sqlite3_step(stmt);
		int count = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		if (count <= 0) { //no data found
			Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
			return;
		}
	}

	//prepare the database stmt
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare(
		database,
		"SELECT * FROM terrain WHERE id = ?;",
		-1,
		&stmt,
		NULL
	);

	if (rc != SQLITE_OK) {
		char buffer[256];
		snprintf(buffer, 256, "SQLite3 returned an error code %d", rc);
		Toy_error(buffer);
		return;
	}

	sqlite3_bind_int(stmt, 1, 1); //id
	sqlite3_step(stmt);

	//free existing terrain, if any
	if (TOY_VALUE_IS_OPAQUE(terrainValue)) {
		free(TOY_VALUE_AS_OPAQUE(terrainValue));
		Toy_freeValue(terrainValue);
	}

	unsigned int width = sqlite3_column_int(stmt, 1);
	unsigned int height = sqlite3_column_int(stmt, 2);

	Terrain* terrain = malloc(sizeof(Terrain) + sizeof(unsigned int) * width * height);
	terrain->type = OPAQUE_TERRAIN;
	terrain->width = width;
	terrain->height = height;
	if (terrain->width * terrain->height > 0) {
		memcpy(terrain->data, (unsigned int*)sqlite3_column_blob(stmt, 3), sizeof(unsigned int) * terrain->width * terrain->height);
	}

	sqlite3_finalize(stmt);

	//leave the reference on the stack
	terrainValue = TOY_OPAQUE_FROM_POINTER(terrain);
	Toy_pushStack(&vm->stack, TOY_REFERENCE_FROM_POINTER(&terrainValue));
}

void api_saveTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	if (terrainLocked) {
		char buffer[256];
		snprintf(buffer, 256, "Can't call 'SaveTerrain' while terrain is locked");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'SaveTerrain'");
		Toy_error(buffer);
		return;
	}

	Toy_Value value = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_OPAQUE(value) || ((Terrain*)TOY_VALUE_AS_OPAQUE(value))->type != OPAQUE_TERRAIN) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'SaveTerrain'");
		Toy_error(buffer);
		Toy_freeValue(value);
		return;
	}

	Terrain* terrain = TOY_VALUE_AS_OPAQUE(value);

	//prepare the database stmt
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare(
		database,
		"INSERT OR REPLACE INTO terrain VALUES (?, ?, ?, ?);",
		-1,
		&stmt,
		NULL
	);

	if (rc != SQLITE_OK) {
		char buffer[256];
		snprintf(buffer, 256, "SQLite3 returned an error code %d", rc);
		Toy_error(buffer);
		return;
	}

	sqlite3_bind_int(stmt, 1, 1); //id
	sqlite3_bind_int(stmt, 2, terrain->width); //width
	sqlite3_bind_int(stmt, 3, terrain->height); //height
	sqlite3_bind_blob(stmt, 4, terrain->data, terrain->width * terrain->height * sizeof(unsigned int), SQLITE_STATIC); //data

	//execute and finish
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void api_getTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	if (TOY_VALUE_IS_OPAQUE(terrainValue)) {
		Toy_pushStack(&vm->stack, TOY_REFERENCE_FROM_POINTER(&terrainValue));
	}
	else {
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
	}
}

void api_setTerrainLockState(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'SetTerrainLockState'");
		Toy_error(buffer);
		return;
	}

	Toy_Value state = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_BOOLEAN(state)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'SetTerrainLockState'");
		Toy_error(buffer);
		Toy_freeValue(state);
		return;
	}

	terrainLocked = TOY_VALUE_AS_BOOLEAN(state);
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"CreateTerrain", api_createTerrain},
	{"UnloadTerrain", api_unloadTerrain},
	{"LoadTerrain", api_loadTerrain},
	{"SaveTerrain", api_saveTerrain},
	// {"GetTerrain", api_getTerrain}, //accessed with 'initTerrainReadOnlyAPI'
	{"SetTerrainLockState", api_setTerrainLockState},
	{NULL, NULL},
};

void initTerrainAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the terrain API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		Toy_declareScope(&vm->memoryBucket, vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}

	//create the table for storage
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare(
		database,
		"CREATE TABLE IF NOT EXISTS terrain (id PRIMARY KEY ON CONFLICT REPLACE, width INTEGER, height INTEGER, data BLOB);",
		-1,
		&stmt,
		NULL
	);

	if (rc != SQLITE_OK) {
		char buffer[256];
		snprintf(buffer, 256, "SQLite3 returned an error code %d", rc);
		Toy_error(buffer);
		return;
	}

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		char buffer[256];
		snprintf(buffer, 256, "SQLite3 failed to initialize the terrain correctly");
		Toy_error(buffer);
		return;
	}

	sqlite3_finalize(stmt);
}

static CallbackPairs readOnlyLockPairs[] = {
	{"GetTerrain", api_getTerrain},
	{NULL, NULL},
};

void initTerrainReadOnlyAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the terrain API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; readOnlyLockPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), readOnlyLockPairs[i].name, strlen(readOnlyLockPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), readOnlyLockPairs[i].callback);

		Toy_declareScope(&vm->memoryBucket, vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}
}

//attributes for terrain
static void attr_terrainSetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	if (terrainLocked) {
		char buffer[256];
		snprintf(buffer, 256, "Can't call 'Terrain.setTile()' while terrain is locked");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 4) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'Terrain.setTile()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value value = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	Terrain* terrain = (Terrain*)TOY_VALUE_AS_OPAQUE(compound);

	//check types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y) || !TOY_VALUE_IS_INTEGER(value)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'Terrain.setTile()'");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//check coordinate bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= terrain->width || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= terrain->height) {
		char buffer[256];
		snprintf(buffer, 256, "Tile coordinates (%d, %d) out of bounds in 'Terrain.setTile()'", TOY_VALUE_AS_INTEGER(x), TOY_VALUE_AS_INTEGER(y));
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//check tile value
	if (!TOY_VALUE_IS_INTEGER(value) || TOY_VALUE_AS_INTEGER(value) < 0) {
		char buffer[256];
		sprintf(buffer, "Bad value found in 'Terrain.setTile()'");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//set the data
	terrain->data[ TOY_VALUE_AS_INTEGER(y) * terrain->width + TOY_VALUE_AS_INTEGER(x) ] = TOY_VALUE_AS_INTEGER(value);
}

static void attr_terrainGetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 3) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'Terrain.getTile()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	Terrain* terrain = (Terrain*)TOY_VALUE_AS_OPAQUE(compound);

	//check types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'Terrain.getTile()'");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//check coordinate bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= terrain->width || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= terrain->height) {
		char buffer[256];
		snprintf(buffer, 256, "Tile coordinates (%d, %d) out of bounds in 'Terrain.getTile()'", TOY_VALUE_AS_INTEGER(x), TOY_VALUE_AS_INTEGER(y));
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//push the value
	Toy_Value value = TOY_VALUE_FROM_INTEGER( (signed)(terrain->data[TOY_VALUE_AS_INTEGER(y) * terrain->width + TOY_VALUE_AS_INTEGER(x)]) );
	Toy_pushStack(&vm->stack, value);
}

Toy_Value handleTerrainAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	Terrain* terrain = (Terrain*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//find the correct operation
	if (CSTR_MATCH(cstr, "width")) {
		return TOY_VALUE_FROM_INTEGER(terrain->width);
	}
	else if (CSTR_MATCH(cstr, "height")) {
		return TOY_VALUE_FROM_INTEGER(terrain->height);
	}
	else if (CSTR_MATCH(cstr, "setTile") && !terrainLocked) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_terrainSetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "getTile")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_terrainGetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown Terrain attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}

Terrain* getTerrainPtr() {
	if (TOY_VALUE_IS_OPAQUE(terrainValue)) {
		return (Terrain*)TOY_VALUE_AS_OPAQUE(terrainValue);
	}
	else {
		return NULL;
	}
}