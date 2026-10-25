#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//fn pointers
typedef bool (*raykey_callback)(int);

//keyboard opaque
typedef struct KeyboardInput {
	OpaqueType type;
	raykey_callback callback;
} KeyboardInput;

extern KeyboardInput keyboardInput;
extern KeyboardInput keyPressedInput;
extern KeyboardInput keyReleasedInput;

Toy_Value handleKeyboardAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);