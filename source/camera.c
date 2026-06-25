#include "camera.h"

#include <stdio.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

//wrapped opaque
CameraData cameraData = {
	.type = OPAQUE_CAMERA,
	.offsetX = 0,
	.offsetY = 0,
	.scaleX = 1,
	.scaleY = 1,
};

//opaque attributes for the camera
static void attr_cameraSetOffsetX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CameraData.setOffsetX()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(x)) {
		x = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(x)));
	}

	if (!TOY_VALUE_IS_FLOAT(x)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in Camera.setOffsetX() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(x.type));
		Toy_error(buffer);
		return;
	}

	CameraData* camera = (CameraData*)TOY_VALUE_AS_OPAQUE(compound);
	camera->offsetX = TOY_VALUE_AS_FLOAT(x);

	Toy_freeValue(compound);
	Toy_freeValue(x);
}

static void attr_cameraSetOffsetY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CameraData.setOffsetY()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(y)) {
		y = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(y)));
	}

	if (!TOY_VALUE_IS_FLOAT(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in Camera.setOffsetY() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		return;
	}

	CameraData* camera = (CameraData*)TOY_VALUE_AS_OPAQUE(compound);
	camera->offsetY = TOY_VALUE_AS_FLOAT(y);

	Toy_freeValue(compound);
	Toy_freeValue(y);
}

static void attr_cameraSetScaleX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CameraData.setScaleX()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(x)) {
		x = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(x)));
	}

	if (!TOY_VALUE_IS_FLOAT(x) || TOY_VALUE_AS_FLOAT(x) <= 0) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in Camera.setScaleX() (expected 'Int' or 'Float' greater than 0, found '%s')", Toy_getValueTypeAsCString(x.type));
		Toy_error(buffer);
		return;
	}

	CameraData* camera = (CameraData*)TOY_VALUE_AS_OPAQUE(compound);
	camera->scaleX = TOY_VALUE_AS_FLOAT(x);

	Toy_freeValue(compound);
	Toy_freeValue(x);
}

static void attr_cameraSetScaleY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CameraData.setScaleY()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(y)) {
		y = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(y)));
	}

	if (!TOY_VALUE_IS_FLOAT(y) || TOY_VALUE_AS_FLOAT(y) <= 0) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in Camera.setScaleX() (expected 'Int' or 'Float' greater than 0, found '%s')", Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		return;
	}

	CameraData* camera = (CameraData*)TOY_VALUE_AS_OPAQUE(compound);
	camera->scaleY = TOY_VALUE_AS_FLOAT(y);

	Toy_freeValue(compound);
	Toy_freeValue(y);
}

Toy_Value handleCameraAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)vm;

	CameraData* camera = (CameraData*)TOY_VALUE_AS_OPAQUE(compound);

	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	if (CSTR_MATCH(cstr, "OFFSET_X")) {
		float result = camera->offsetX;
		return TOY_VALUE_FROM_FLOAT(result);
	}
	else if (CSTR_MATCH(cstr, "OFFSET_Y")) {
		float result = camera->offsetY;
		return TOY_VALUE_FROM_FLOAT(result);
	}
	else if (CSTR_MATCH(cstr, "SCALE_X")) {
		float result = camera->scaleX;
		return TOY_VALUE_FROM_FLOAT(result);
	}
	else if (CSTR_MATCH(cstr, "SCALE_Y")) {
		float result = camera->scaleY;
		return TOY_VALUE_FROM_FLOAT(result);
	}

	else if (CSTR_MATCH(cstr, "setOffsetX")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_cameraSetOffsetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setOffsetY")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_cameraSetOffsetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setScaleX")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_cameraSetScaleX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setScaleY")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_cameraSetScaleY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		//unknown
		char buffer[256];
		snprintf(buffer, 256, "Unknown camera attribute '%s'", cstr);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}