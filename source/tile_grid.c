#include "tile_grid.h"
#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TileGridData tileGridData = {
	.type = OPAQUE_TILE_GRID,
};

//static members
unsigned int gridWidth = 0;
unsigned int gridHeight = 0;
unsigned int *gridContents = NULL;

void api_initTileGrid(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value initial = Toy_popStack(&vm->stack);
	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		char buffer[256];
		sprintf(buffer, "Bad argument types found in 'initTileGrid' (width & height)");
		Toy_error(buffer);
		return;
	}

	if (!TOY_VALUE_IS_ARRAY(initial) && !TOY_VALUE_IS_NULL(initial)) {
		char buffer[256];
		sprintf(buffer, "Bad argument types found in 'initTileGrid' (initial array)");
		Toy_error(buffer);
		return;
	}

	if (TOY_VALUE_AS_INTEGER(width) <= 0 || TOY_VALUE_AS_INTEGER(height) <= 0) {
		char buffer[256];
		sprintf(buffer, "Bad argument values found in 'initTileGrid' ('width' & 'height' must be greater than '0')");
		Toy_error(buffer);
		return;
	}

	gridWidth = TOY_VALUE_AS_INTEGER(width);
	gridHeight = TOY_VALUE_AS_INTEGER(height);

	gridContents = malloc(gridWidth * gridHeight * sizeof(unsigned int));
	memset(gridContents, 0, gridWidth * gridHeight * sizeof(unsigned int));

	if (TOY_VALUE_IS_NULL(initial)) {
		Toy_freeValue(width);
		Toy_freeValue(height);
		Toy_freeValue(initial);
		return;
	}

	Toy_Array* array = TOY_VALUE_AS_ARRAY(initial);

	//TODO: add 'array.flatten()' or something to standard library
	if (array->count != gridWidth*gridHeight) {
		char buffer[256];
		sprintf(buffer, "Array 'initial' found in 'initTileGrid' has the wrong number of elements; Expecte a 1d array of %u elements, found %u elements instead", gridWidth*gridHeight, array->count);
		Toy_error(buffer);
		Toy_freeValue(width);
		Toy_freeValue(height);
		Toy_freeValue(initial);
		return;
	}

	for (unsigned int i = 0; i < gridWidth*gridHeight; i++) {
		if (!TOY_VALUE_IS_INTEGER(array->data[i]) || TOY_VALUE_AS_INTEGER(array->data[i]) < 0) {
			char buffer[256];
			sprintf(buffer, "An element at index %u in the array 'initial' found in 'initTileGrid' is not an integer greater than or equal to '0' (tile ignored)", i);
			Toy_error(buffer);
			continue;
		}

		gridContents[i] = TOY_VALUE_AS_INTEGER(array->data[i]);
	}

	//cleanup
	Toy_freeValue(width);
	Toy_freeValue(height);
	Toy_freeValue(initial);
}

//opaque attributes
static void attr_tileGridSetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check for initialization
	if (gridContents == NULL) {
		char buffer[256];
		sprintf(buffer, "'TileGrid' not initialized");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack); //the grid is always on top here
	Toy_Value value = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	//types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y) || !TOY_VALUE_IS_INTEGER(value)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'TileGrid.setTile()'");
		Toy_error(buffer);
		return;
	}

	//value bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || TOY_VALUE_AS_INTEGER(value) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= gridWidth || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= gridHeight) {
		char buffer[256];
		snprintf(buffer, 256, "Bad values found in 'TileGrid.setTile()'");
		Toy_error(buffer);
		return;
	}

	//update the grid
	gridContents[ TOY_VALUE_AS_INTEGER(y) * gridWidth + TOY_VALUE_AS_INTEGER(x) ] = TOY_VALUE_AS_INTEGER(value);

	//cleanup
	Toy_freeValue(compound);
	Toy_freeValue(x);
	Toy_freeValue(y);
	Toy_freeValue(value);
}

static void attr_tileGridGetTile(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check for initialization
	if (gridContents == NULL) {
		char buffer[256];
		sprintf(buffer, "'TileGrid' not initialized");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack); //the grid is always on top here
	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	//types
	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad types found in 'TileGrid.getTile()'");
		Toy_error(buffer);
		return;
	}

	//value bounds
	if (TOY_VALUE_AS_INTEGER(x) < 0 || TOY_VALUE_AS_INTEGER(y) < 0 || (unsigned int)TOY_VALUE_AS_INTEGER(x) >= gridWidth || (unsigned int)TOY_VALUE_AS_INTEGER(y) >= gridHeight) {
		char buffer[256];
		snprintf(buffer, 256, "Bad values found in 'TileGrid.getTile()'");
		Toy_error(buffer);
		return;
	}

	//push the value
	Toy_Value value = TOY_VALUE_FROM_INTEGER( (signed int)gridContents[TOY_VALUE_AS_INTEGER(y) * gridWidth + TOY_VALUE_AS_INTEGER(x)] );
	Toy_pushStack(&vm->stack, value);

	//cleanup
	Toy_freeValue(compound);
	Toy_freeValue(value);
	Toy_freeValue(x);
	Toy_freeValue(y);
}

Toy_Value handleTileGridAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)compound; //this doesn't care about the object, as there's only one grid

	//check for initialization
	if (gridContents == NULL) {
		char buffer[256];
		sprintf(buffer, "'TileGrid' not initialized");
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}

	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//NOTE: this probably could use a loop too, but its short enough that IDC.

	//find the correct operation
	if (strlen(cstr) == 5 && strncmp(cstr, "width", strlen(cstr)) == 0) {
		return TOY_VALUE_FROM_INTEGER(gridWidth);
	}
	else if (strlen(cstr) == 6 && strncmp(cstr, "height", strlen(cstr)) == 0) {
		return TOY_VALUE_FROM_INTEGER(gridHeight);
	}

	else if (strlen(cstr) == 7 && strncmp(cstr, "setTile", strlen(cstr)) == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridSetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else if (strlen(cstr) == 7 && strncmp(cstr, "getTile", strlen(cstr)) == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_tileGridGetTile);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown TileGrid attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}
