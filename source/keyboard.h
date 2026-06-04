#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//fn pointers
typedef bool (*raykey_callback)(int);

//keyboard opaque
typedef struct KeyboardData {
	OpaqueType type;
	raykey_callback callback;
} KeyboardData;

extern KeyboardData keyboardData;
extern KeyboardData keyPressedData;
extern KeyboardData keyReleasedData;

Toy_Value handleKeyboardAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);