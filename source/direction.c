#include "direction.h"

#include <stdio.h>
#include <string.h>

typedef struct DirectionMap {
	Direction dir;
	char* cstr;
} DirectionMap;

DirectionMap directionMap[] = {
	{NORTH, "NORTH"},
	{NORTHEAST, "NORTHEAST"},
	{EAST, "EAST"},
	{SOUTHEAST, "SOUTHEAST"},
	{SOUTH, "SOUTH"},
	{SOUTHWEST, "SOUTHWEST"},
	{WEST, "WEST"},
	{NORTHWEST, "NORTHWEST"},
};

DirectionData directionData = {
	.type = OPAQUE_DIRECTION,
};

#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

Toy_Value handleDirectionAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)vm;
	(void)compound;

	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//find the mapped value, if available
	for (DirectionMap* ptr = directionMap; ptr->cstr != NULL; ptr++) {
		if (CSTR_MATCH(ptr->cstr, cstr)) {
			return TOY_VALUE_FROM_INTEGER(ptr->dir);
		}
	}

	//unknown
	char buffer[256];
	snprintf(buffer, 256, "Unknown direction attribute '%s'", cstr);
	Toy_error(buffer);
	return TOY_VALUE_FROM_NULL();
}