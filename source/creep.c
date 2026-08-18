#include "creep.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

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
	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'Creep' requested in 'handleCreepAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}