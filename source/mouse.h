#pragma once

#include "opaque_type.h"
#include "toy_vm.h"
#include "raylib.h"

//fn pointers
typedef bool (*raymouse_callback)(int);

//mouse opaque
typedef struct MouseData {
	OpaqueType type;
	raymouse_callback callback;
} MouseData;

extern MouseData mouseData;
extern MouseData mousePressedData;
extern MouseData mouseReleasedData;

Toy_Value handleMouseAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);
