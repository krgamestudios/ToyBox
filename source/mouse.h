#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//fn pointers
typedef bool (*raymouse_callback)(int);

//mouse opaque
typedef struct MouseInput {
	OpaqueType type;
	raymouse_callback callback;
} MouseInput;

extern MouseInput mouseInput;
extern MouseInput mousePressedInput;
extern MouseInput mouseReleasedInput;

Toy_Value handleMouseAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
