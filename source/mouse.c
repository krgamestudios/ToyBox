#include "mouse.h"

#include <stdio.h>
#include <string.h>

MouseData mouseData = {
	.type = OPAQUE_MOUSE,
	.callback = &IsMouseButtonDown,
};

MouseData mousePressedData = {
	.type = OPAQUE_MOUSE_PRESSED,
	.callback = &IsMouseButtonPressed,
};

MouseData mouseReleasedData = {
	.type = OPAQUE_MOUSE_PRESSED,
	.callback = &IsMouseButtonReleased,
};

#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

Toy_Value handleMouseAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)vm;

	MouseData* md = (MouseData*)TOY_VALUE_AS_OPAQUE(compound);

	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//NOTE: this probably could use a loop too, but its short enough that IDC.

	//find the correct operation
	if (CSTR_MATCH(cstr, "LEFT")) {
		bool result = md->callback(MOUSE_BUTTON_LEFT);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (CSTR_MATCH(cstr, "RIGHT")) {
		bool result = md->callback(MOUSE_BUTTON_RIGHT);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (CSTR_MATCH(cstr, "MIDDLE")) {
		bool result = md->callback(MOUSE_BUTTON_MIDDLE);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (CSTR_MATCH(cstr, "POS_X")) {
		int result = GetMouseX();
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (CSTR_MATCH(cstr, "POS_Y")) {
		int result = GetMouseY();
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (CSTR_MATCH(cstr, "MOVE_X")) {
		int result = GetMouseDelta().x;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (CSTR_MATCH(cstr, "MOVE_Y")) {
		int result = GetMouseDelta().y;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (CSTR_MATCH(cstr, "WHEEL_X")) {
		int result = GetMouseWheelMoveV().x;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (CSTR_MATCH(cstr, "WHEEL_Y")) {
		int result = GetMouseWheelMoveV().y;
		return TOY_VALUE_FROM_INTEGER(result);
	}

	else {
		//unknown
		char buffer[256];
		snprintf(buffer, 256, "Unknown mouse attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}
