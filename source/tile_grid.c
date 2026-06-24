#include "tile_grid.h"
#include "camera.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

static void api_loadTileSet(Toy_VM* vm, Toy_FunctionNative* self) {
	//file name, tile width, tile height -> Opaque(TileSet)
	(void)self;

	if (!IsWindowReady()) {
		char buffer[256];
		snprintf(buffer, 256, "Can't load tilesets before the window has been initialized");
		Toy_error(buffer);
		return;
	}

	if (vm->stack->count < 3) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'loadTileSet'");
		Toy_error(buffer);
		return;
	}

	Toy_Value tileHeight = Toy_popStack(&vm->stack);
	Toy_Value tileWidth = Toy_popStack(&vm->stack);
	Toy_Value file = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_STRING(file) || !TOY_VALUE_IS_INTEGER(tileWidth) || !TOY_VALUE_IS_INTEGER(tileHeight)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'loadTileSet'");
		Toy_error(buffer);
		Toy_freeValue(file);
		Toy_freeValue(tileWidth);
		Toy_freeValue(tileHeight);
		return;
	}

	TileSetData* tileSetData = (TileSetData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(TileSetData));
	tileSetData->type = OPAQUE_TILE_SET;

	//load the texture from a file
	Toy_String* str = TOY_VALUE_AS_STRING(file);
	if (str->info.type == TOY_STRING_LEAF) {
		tileSetData->texture = LoadTexture(str->leaf.data);
	}
	else {
		char* cstr = Toy_getStringRaw(str);
		tileSetData->texture = LoadTexture(cstr);
		free(cstr);
	}

	tileSetData->tileWidth = TOY_VALUE_AS_INTEGER(tileWidth);
	tileSetData->tileHeight = TOY_VALUE_AS_INTEGER(tileHeight);

	Toy_Value tileSetValue = TOY_OPAQUE_FROM_POINTER(tileSetData);
	Toy_pushStack(&vm->stack, tileSetValue);

	Toy_freeValue(file);
	Toy_freeValue(tileWidth);
	Toy_freeValue(tileHeight);
}

void api_createTileGrid(Toy_VM* vm, Toy_FunctionNative* self) {
	//width, height, initial state -> Opaque(TileGrid)
	(void)self;

	Toy_Value initial = Toy_popStack(&vm->stack);
	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		char buffer[256];
		sprintf(buffer, "Bad argument types found in 'createTileGrid' (width & height)");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	if (!TOY_VALUE_IS_ARRAY(initial) && !TOY_VALUE_IS_NULL(initial)) {
		char buffer[256];
		sprintf(buffer, "Bad argument types found in 'createTileGrid' (initial array)");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	if (TOY_VALUE_AS_INTEGER(width) <= 0 || TOY_VALUE_AS_INTEGER(height) <= 0) {
		char buffer[256];
		sprintf(buffer, "Bad argument values found in 'createTileGrid' ('width' & 'height' must be greater than '0')");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//make the grid object
	TileGridData *tileGridData = (TileGridData*)Toy_partitionBucket(&vm->memoryBucket, sizeof(TileGridData));
	(*tileGridData) = (TileGridData){
		.type = OPAQUE_TILE_GRID,
		.width = TOY_VALUE_AS_INTEGER(width),
		.height = TOY_VALUE_AS_INTEGER(height),
		.cells = NULL,
	};

	tileGridData->cells = malloc(tileGridData->width * tileGridData->height * sizeof(unsigned int));
	memset(tileGridData->cells, 0, tileGridData->width * tileGridData->height * sizeof(unsigned int));

	//if not initialized, clean up and return
	if (TOY_VALUE_IS_NULL(initial)) {
		Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(tileGridData));
		Toy_freeValue(width);
		Toy_freeValue(height);
		Toy_freeValue(initial);
		return;
	}

	//set the internal state
	Toy_Array* array = TOY_VALUE_AS_ARRAY(initial);

	if (array->count != tileGridData->width*tileGridData->height) {
		char buffer[256];
		sprintf(buffer, "Array 'initial' found in 'createTileGrid' has the wrong number of elements; Expecte a 1d array of %u elements, found %u elements instead", tileGridData->width*tileGridData->height, array->count);
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(tileGridData));
		Toy_freeValue(width);
		Toy_freeValue(height);
		Toy_freeValue(initial);
		return;
	}

	for (unsigned int i = 0; i < tileGridData->width*tileGridData->height; i++) {
		if (!TOY_VALUE_IS_INTEGER(array->data[i]) || TOY_VALUE_AS_INTEGER(array->data[i]) < 0) {
			char buffer[256];
			sprintf(buffer, "An element at index %u in the array 'initial' found in 'createTileGrid' is not an integer greater than or equal to '0' (tile ignored)", i);
			Toy_error(buffer);
			continue;
		}

		tileGridData->cells[i] = TOY_VALUE_AS_INTEGER(array->data[i]);
	}

	//return
	Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(tileGridData));

	//cleanup
	Toy_freeValue(width);
	Toy_freeValue(height);
	Toy_freeValue(initial);
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"LoadTileSet", api_loadTileSet},
	{"CreateTileGrid", api_createTileGrid},
	{NULL, NULL},
};

void initTileGridAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the actor API, exiting\n" TOY_CC_RESET);
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

//opaque attributes for tile sets
static void attr_tileSetUnload(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	TileSetData* tileSetData = (TileSetData*)TOY_VALUE_AS_OPAQUE(compound);

	UnloadTexture(tileSetData->texture);
	Toy_releaseBucketPartition((void*)tileSetData);

	//WARN: the variable name will be invalid from this point onwards, I think?
}

Toy_Value handleTileSetAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	TileSetData* tileSetData = (TileSetData*)TOY_VALUE_AS_OPAQUE(compound);

	(void)vm;

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//find the correct operation
	if (CSTR_MATCH(cstr, "tileWidth")) {
		return TOY_VALUE_FROM_INTEGER(tileSetData->tileWidth);
	}
	else if (CSTR_MATCH(cstr, "tileHeight")) {
		return TOY_VALUE_FROM_INTEGER(tileSetData->tileHeight);
	}
	else if (CSTR_MATCH(cstr, "unload")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileSetUnload);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown TileSet attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}

//opaque attributes for tile grids
static void attr_tileGridSetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value value = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	TileGridData* tileGridData = (TileGridData*)TOY_VALUE_AS_OPAQUE(compound);

	//types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y) || !TOY_VALUE_IS_INTEGER(value)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'TileGrid.setTile()'");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//value bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= tileGridData->width || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= tileGridData->height) {
		char buffer[256];
		snprintf(buffer, 256, "Tile coordinates (%d, %d) out of bounds in 'TileGrid.setTile()'", TOY_VALUE_AS_INTEGER(x), TOY_VALUE_AS_INTEGER(y));
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	if (!TOY_VALUE_IS_INTEGER(value) || TOY_VALUE_AS_INTEGER(value) < 0) {
			char buffer[256];
			sprintf(buffer, "Tile value found in 'TileGrid.setTile()' is not an integer greater than or equal to '0'");
			Toy_error(buffer);
			Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
			return;
		}

	//update the grid
	tileGridData->cells[ TOY_VALUE_AS_INTEGER(y) * tileGridData->width + TOY_VALUE_AS_INTEGER(x) ] = TOY_VALUE_AS_INTEGER(value);

	//cleanup
	Toy_freeValue(compound);
	Toy_freeValue(x);
	Toy_freeValue(y);
	Toy_freeValue(value);
}

static void attr_tileGridGetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack); //the grid is always on top here
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	TileGridData* tileGridData = (TileGridData*)TOY_VALUE_AS_OPAQUE(compound);

	//types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'TileGrid.getTile()'");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//value bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= tileGridData->width || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= tileGridData->height) {
		char buffer[256];
		snprintf(buffer, 256, "Tile coordinates (%d, %d) out of bounds in 'TileGrid.getTile()'", TOY_VALUE_AS_INTEGER(x), TOY_VALUE_AS_INTEGER(y));
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//push the value
	Toy_Value value = TOY_VALUE_FROM_INTEGER( (signed int)(tileGridData->cells[TOY_VALUE_AS_INTEGER(y) * tileGridData->width + TOY_VALUE_AS_INTEGER(x)]) );
	Toy_pushStack(&vm->stack, value);

	//cleanup
	Toy_freeValue(compound);
	Toy_freeValue(value);
	Toy_freeValue(x);
	Toy_freeValue(y);
}

static void attr_tileGridDrawToScreen(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value tileGridValue = Toy_popStack(&vm->stack);
	Toy_Value tileSetValue = Toy_popStack(&vm->stack);
	Toy_Value posY = Toy_popStack(&vm->stack);
	Toy_Value posX = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(posX)) posX = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(posX)));
	if (TOY_VALUE_IS_INTEGER(posY)) posY = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(posY)));

	//types
	if (!TOY_VALUE_IS_FLOAT(posX) || !TOY_VALUE_IS_FLOAT(posY) || !TOY_VALUE_IS_OPAQUE(tileSetValue)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'TileGrid.drawToScreen()'");
		Toy_error(buffer);
		Toy_freeValue(tileGridValue);
		Toy_freeValue(posX);
		Toy_freeValue(posY);
		Toy_freeValue(tileSetValue);
		return;
	}

	//useable variables for convenience
	TileGridData* tileGridData = (TileGridData*)TOY_VALUE_AS_OPAQUE(tileGridValue);
	TileSetData* tileSetData = (TileSetData*)TOY_VALUE_AS_OPAQUE(tileSetValue);
	unsigned int tileSetXCount = tileSetData->texture.width / tileSetData->tileWidth;
	unsigned int tileSetYCount = tileSetData->texture.height / tileSetData->tileHeight;

	(void)tileSetYCount;

	//
	for (unsigned int j = 0; j < tileGridData->height; j++) {
		for (unsigned int i = 0; i < tileGridData->width; i++) {
			//the tile to be drawn
			unsigned int tileData = tileGridData->cells[j * tileGridData->width + i];

			//Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint
			DrawTexturePro(
				tileSetData->texture,
				(Rectangle){
					(tileData % tileSetXCount) * tileSetData->tileWidth,
					(tileData / tileSetXCount) * tileSetData->tileHeight,
					tileSetData->tileWidth,
					tileSetData->tileHeight,
				},
				(Rectangle){
					(TOY_VALUE_AS_FLOAT(posX) - cameraData.offsetX + i * tileSetData->tileWidth) * cameraData.scaleX,
					(TOY_VALUE_AS_FLOAT(posY) - cameraData.offsetY + j * tileSetData->tileHeight) * cameraData.scaleY,
					tileSetData->tileWidth * cameraData.scaleX,
					tileSetData->tileHeight * cameraData.scaleY,
				},
				(Vector2){0,0}, //top-left
				0,
				WHITE
			);
		}
	}
}

static void attr_tileGridUnload(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	TileGridData* tileGridData = (TileGridData*)TOY_VALUE_AS_OPAQUE(compound);

	free(tileGridData->cells);
	Toy_releaseBucketPartition((void*)tileGridData);

	//WARN: the variable name will be invalid from this point onwards, I think?
}

Toy_Value handleTileGridAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//useable grid
	TileGridData* tileGridData = (TileGridData*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//NOTE: this probably could use a loop too, but its short enough that IDC.

	//find the correct operation
	if (CSTR_MATCH(cstr, "width")) {
		return TOY_VALUE_FROM_INTEGER(tileGridData->width);
	}
	else if (CSTR_MATCH(cstr, "height")) {
		return TOY_VALUE_FROM_INTEGER(tileGridData->height);
	}
	else if (CSTR_MATCH(cstr, "setTile")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridSetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "getTile")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridGetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "drawToScreen")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridDrawToScreen);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "unload")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridUnload);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown TileGrid attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}
