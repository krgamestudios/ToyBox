#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

//camera opaque
typedef struct CameraData {
	OpaqueType type;
	float offsetX;
	float offsetY;
	float scaleX;
	float scaleY;
} CameraData;

extern CameraData cameraData;

Toy_Value handleCameraAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);