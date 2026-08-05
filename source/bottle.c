#include "bottle.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

static void api_spawnBottleAt(Toy_VM* vm, Toy_FunctionNative* self) {
	//x, y -> Opaque(BottleData)
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'spawnBottleAt'");
		Toy_error(buffer);
		return;
	}

	Toy_Value ypos = Toy_popStack(&vm->stack);
	Toy_Value xpos = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(xpos)) {
		xpos = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(xpos)));
	}
	if (TOY_VALUE_IS_INTEGER(ypos)) {
		ypos = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(ypos)));
	}

	//check types
	if (!TOY_VALUE_IS_FLOAT(xpos) || !TOY_VALUE_IS_FLOAT(ypos)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'spawnBottleAt'");
		Toy_error(buffer);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	BottleData* bottle = (BottleData*)Toy_partitionBucket(&vm->memoryBucket, sizeof(BottleData));

	(*bottle) = (BottleData){
		.type = OPAQUE_BOTTLE_DATA,
		.position = { TOY_VALUE_AS_FLOAT(xpos), TOY_VALUE_AS_FLOAT(ypos) },
	};

	//leave the reference on the stack
	Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(bottle));
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"SpawnBottleAt", api_spawnBottleAt},
	{NULL, NULL},
};

//exposed
void initBottleAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the bottle API, exiting\n" TOY_CC_RESET);
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

void freeBottleAPI(Toy_VM* vm) {
	(void)vm;
	//NO-OP
}

//opaque attributes
static void attr_bottleSetX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value param = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(param)) {
		param = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(param)));
	}

	if (!TOY_VALUE_IS_FLOAT(param)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in BottleData.setX() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(param.type));
		Toy_error(buffer);
		Toy_freeValue(param);
		return;
	}

	BottleData* bottle = (BottleData*)TOY_VALUE_AS_OPAQUE(compound);
	bottle->position.x = TOY_VALUE_AS_FLOAT(param);
}

static void attr_bottleSetY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value param = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(param)) {
		param = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(param)));
	}

	if (!TOY_VALUE_IS_FLOAT(param)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in BottleData.setY() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(param.type));
		Toy_error(buffer);
		Toy_freeValue(param);
		return;
	}

	BottleData* bottle = (BottleData*)TOY_VALUE_AS_OPAQUE(compound);
	bottle->position.y = TOY_VALUE_AS_FLOAT(param);
}

static void attr_bottleDespawn(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack); //compound is (presumably) a reference

	if (!TOY_VALUE_IS_OPAQUE(compound)) {
		char buffer[256];
		snprintf(buffer, 256, "Bottle can't be despawned in 'BottleData.despawn()', expected value type %s, found %s", Toy_getValueTypeAsCString(TOY_VALUE_OPAQUE), Toy_getValueTypeAsCString(compound.type));
		Toy_error(buffer);
		Toy_freeValue(compound);
		return;
	}

	//clear the data within the bucket
	BottleData* bottle = (BottleData*)TOY_VALUE_AS_OPAQUE(compound);

	Toy_releaseBucketPartition((void*)bottle); //TODO: move to an array for each player

	Toy_freeValue(compound); //BUG: doesn't actually free it correctly
}

Toy_Value handleBottleAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//useable object
	BottleData* bottle = (BottleData*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute's name
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	if (CSTR_MATCH(cstr, "x")) {
		return TOY_VALUE_FROM_INTEGER(bottle->position.x);
	}
	else if (CSTR_MATCH(cstr, "y")) {
		return TOY_VALUE_FROM_INTEGER(bottle->position.y);
	}
	else if (CSTR_MATCH(cstr, "setX")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_bottleSetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setY")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_bottleSetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "despawn")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_bottleDespawn);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'BottleData' requested in 'handleBottleAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}