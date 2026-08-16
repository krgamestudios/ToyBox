#include "player.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//API for manipulating the game as a player
static void api_createCreep(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	//check parameter count
	if (vm->stack->count < 2) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'CreateCreep()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value y = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(x) || !TOY_VALUE_IS_INTEGER(y)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument types found in CreateCreep() (expected 'Int' and 'Int',  found '%s' and '%s')", Toy_getValueTypeAsCString(x.type), Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		Toy_freeValue(x);
		Toy_freeValue(y);
		return;
	}

	//TODO: place the creep within the player's array
	Creep* creep = (Creep*)Toy_partitionBucket(&vm->memoryBucket, sizeof(Creep));

	(*creep) = (Creep){
		.type = OPAQUE_CREEP,
		.position = (Vector2){ .x = TOY_VALUE_AS_INTEGER(x), .y = TOY_VALUE_AS_INTEGER(y)}
	};

	Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(creep));
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"CreateCreep", api_createCreep},
	{NULL, NULL},
};

void initPlayerAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the creep API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		Toy_declareScope(&vm->memoryBucket, vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}

	//TODO: sqlite3 storage for creeps
}

Player* allocatePlayer() {
	Toy_VM vm;
	Toy_initVM(&vm);
	Player* player = (Player*)Toy_partitionBucket(&vm.memoryBucket, sizeof(Player));
	player->vm = vm;
	player->creepCapacity = 8; //initial capacity
	player->creepCount = 0;
	player->creeps = (Creep*)Toy_partitionBucket(&vm.memoryBucket, sizeof(Creep) * player->creepCapacity);
	return player;
}

void bindBytecodeToPlayer(Player* player, unsigned char* bytecode) {
	Toy_bindVM(&player->vm, bytecode, NULL);
}

void freePlayer(Player* player) {
	//free any bytecode, if able
	if (player->vm.code) {
		free(player->vm.code);
	}

	//TODO: do any cleanup needed
	Toy_VM vm = player->vm; //take ownership away from the player object

	//this will also free the player's allocated memory within the bucket
	Toy_freeVM(&vm);
}
