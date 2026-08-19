#pragma once

#include "opaque_type.h"
#include "toy_vm.h"

typedef enum Direction {
	NONE = 0,
	NORTH,
	NORTHEAST,
	EAST,
	SOUTHEAST,
	SOUTH,
	SOUTHWEST,
	WEST,
	NORTHWEST,
} Direction;

typedef struct DirectionData {
	OpaqueType type;
} DirectionData;

extern DirectionData directionData;

Toy_Value handleDirectionAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);