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

Toy_Value handleMouseAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)vm;

	MouseData* md = (MouseData*)TOY_VALUE_AS_OPAQUE(compound);

	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	//NOTE: this probably could use a loop too, but its short enough that IDC.

	//find the correct operation
	if (strlen(cstr) == 4 && strncmp(cstr, "LEFT", strlen(cstr)) == 0) {
		bool result = md->callback(MOUSE_BUTTON_LEFT);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (strlen(cstr) == 5 && strncmp(cstr, "RIGHT", strlen(cstr)) == 0) {
		bool result = md->callback(MOUSE_BUTTON_RIGHT);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (strlen(cstr) == 6 && strncmp(cstr, "MIDDLE", strlen(cstr)) == 0) {
		bool result = md->callback(MOUSE_BUTTON_MIDDLE);
		return TOY_VALUE_FROM_BOOLEAN(result);
	}
	else if (strlen(cstr) == 5 && strncmp(cstr, "POS_X", strlen(cstr)) == 0) {
		int result = GetMouseX();
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (strlen(cstr) == 5 && strncmp(cstr, "POS_Y", strlen(cstr)) == 0) {
		int result = GetMouseY();
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (strlen(cstr) == 6 && strncmp(cstr, "MOVE_X", strlen(cstr)) == 0) {
		int result = GetMouseDelta().x;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (strlen(cstr) == 6 && strncmp(cstr, "MOVE_Y", strlen(cstr)) == 0) {
		int result = GetMouseDelta().y;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (strlen(cstr) == 7 && strncmp(cstr, "WHEEL_X", strlen(cstr)) == 0) {
		int result = GetMouseWheelMoveV().x;
		return TOY_VALUE_FROM_INTEGER(result);
	}
	else if (strlen(cstr) == 7 && strncmp(cstr, "WHEEL_Y", strlen(cstr)) == 0) {
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
