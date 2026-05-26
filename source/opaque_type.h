#pragma once

//always first member of any opaques
typedef enum OpaqueType {
	OPAQUE_KEYBOARD,
	OPAQUE_ACTOR,
} OpaqueType;

typedef struct DummyOpaque {
	OpaqueType type;
} DummyOpaque;