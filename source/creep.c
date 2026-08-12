#include "creep.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

static void api_createCreep(Toy_VM* vm, Toy_FunctionNative* self) {
	//TODO: store this creep within the Player
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CreateCreep()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument types found in CreateCreep() (expected 'Int' and 'Int',  found '%s' and '%s')", Toy_getValueTypeAsCString(x.type), Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		Toy_freeValue(x);
		Toy_freeValue(y);
		return;
	}

	//allocate space for a value and a creep
	Creep* creep = (Creep*)Toy_partitionBucket(&vm->memoryBucket, sizeof(Creep));

	(*creep) = (Creep){
		.type = OPAQUE_CREEP,
		.position = (Vector2){ .x = TOY_VALUE_AS_INTEGER(x), .y = TOY_VALUE_AS_INTEGER(y)}
	};

	//TODO: I need a global 'nullptr' for references/opaques?
	Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(creep));
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"CreateCreep", api_createCreep},
	{NULL, NULL},
};

void initCreepAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the creep API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		Toy_declareScope(&vm->memoryBucket, vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}

	//TODO: sqlite3 storage for creeps
}

static void attr_creepSetX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'Creep.setX()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(x)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type found in Creep.setX() (expected 'Int' found '%s')", Toy_getValueTypeAsCString(x.type));
		Toy_error(buffer);
		Toy_freeValue(x);
		return;
	}

	Creep* creep = (Creep*)TOY_VALUE_AS_OPAQUE(compound);
	creep->position.x = TOY_VALUE_AS_INTEGER(x);
}

static void attr_creepSetY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'Creep.setY()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type found in Creep.setY() (expected 'Int' found '%s')", Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		Toy_freeValue(y);
		return;
	}

	Creep* creep = (Creep*)TOY_VALUE_AS_OPAQUE(compound);
	creep->position.y = TOY_VALUE_AS_INTEGER(y);
}

static void attr_creepDestroy(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'Creep.destroy()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_OPAQUE(compound)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type found in Creep.destroy() (expected 'Opaque' found '%s')", Toy_getValueTypeAsCString(compound.type));
		Toy_error(buffer);
		Toy_freeValue(compound);
		return;
	}

	Creep* creep = TOY_VALUE_AS_OPAQUE(compound);
	Toy_releaseBucketPartition((void*)creep);

	//nullify the scoped variable
	if (TOY_VALUE_IS_REFERENCE(compound)) {
		(*TOY_VALUE_AS_REFERENCE(compound)) = TOY_VALUE_FROM_NULL();
	}
}

Toy_Value handleCreepAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	Creep* creep = (Creep*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	if (CSTR_MATCH(cstr, "x")) {
		return TOY_VALUE_FROM_INTEGER(creep->position.x);
	}
	else if (CSTR_MATCH(cstr, "y")) {
		return TOY_VALUE_FROM_INTEGER(creep->position.y);
	}
	else if (CSTR_MATCH(cstr, "setX")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_creepSetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setY")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_creepSetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "destroy")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_creepDestroy);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'Creep' requested in 'handleCreepAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}