#pragma once

//always first member of any opaques
typedef enum OpaqueType {
	OPAQUE_KEYBOARD,
	OPAQUE_KEY_PRESSED,
	OPAQUE_KEY_RELEASED,
	OPAQUE_MOUSE,
	OPAQUE_MOUSE_PRESSED,
	OPAQUE_MOUSE_RELEASED,
	OPAQUE_ACTOR,
	OPAQUE_TILE_GRID,
} OpaqueType;
