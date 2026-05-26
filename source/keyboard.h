#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//wraps raylib's 'KeyboardKey' enum to a c-string
typedef struct KeyboardMap {
	int raykey;
	char* cstr;
} KeyboardMap;

extern KeyboardMap keyboardMap[];

//keyboard opaque
typedef struct KeyboardData {
	OpaqueType type;
} KeyboardData;

extern KeyboardData keyboardData;

Toy_Value handleKeyboardAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);