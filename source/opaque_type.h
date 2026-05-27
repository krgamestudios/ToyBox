#pragma once

//always first member of any opaques
typedef enum OpaqueType {
	OPAQUE_KEYBOARD,
	OPAQUE_KEY_PRESSED,
	OPAQUE_KEY_RELEASED,
	OPAQUE_ACTOR,
} OpaqueType;
