#include "terrain.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

//static members
static Toy_Value terrainValue = {0};

void api_createTerrain(Toy_VM* vm, Toy_FunctionNative* self) {
    //width, height -> Opaque(Terrain)
	(void)self;

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
	}
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"CreateTerrain", api_createTerrain},
	{"UnloadTerrain", api_unloadTerrain},
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

		Toy_declareScope(vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}
}

//attributes for terrain
static void attr_terrainSetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

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
    (void)vm;

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
	else if (CSTR_MATCH(cstr, "setTile")) {
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