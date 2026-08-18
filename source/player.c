#include "player.h"

#include "toy_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//API for manipulating the game as a player
static void api_createCreep(Toy_VM* vm, Toy_FunctionNative* self) {
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

	//the player object
	void* ptr = (void*)(&self->meta1);
	Player* player = *((Player**)ptr);

	if (player->creepCount >= player->creepCapacity) {
		char buffer[256];
		snprintf(buffer, 256, "Too many creeps in player's data");
		Toy_error(buffer);
		Toy_pushStack(&vm->stack, TOY_VALUE_FROM_NULL());
		return;
	}

	//find an unused slot for this creep
	Creep* creep = NULL;
	for (unsigned int i = 0; i < player->creepCapacity; i++) {
		if (player->creeps[i].active != true) {
			creep = &player->creeps[i];
			break;
		}
	}
	player->creepCount++;

	(*creep) = (Creep){
		.type = OPAQUE_CREEP,
		.active = true,
		.position = (Vector2){ .x = TOY_VALUE_AS_INTEGER(x), .y = TOY_VALUE_AS_INTEGER(y)}
	};

	Toy_pushStack(&vm->stack, TOY_OPAQUE_FROM_POINTER(creep));
}

static void api_destroyCreep(Toy_VM* vm, Toy_FunctionNative* self) {
	if (vm->stack->count < 1) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'DestroyCreep()'");
		Toy_error(buffer);
		return;
	}

	Toy_Value value = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_OPAQUE(value)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type found in DestroyCreep() (expected 'Opaque' found '%s')", Toy_getValueTypeAsCString(value.type));
		Toy_error(buffer);
		Toy_freeValue(value);
		return;
	}

	//the player & creep objects
	void* ptr = (void*)(&self->meta1);
	Player* player = *((Player**)ptr);
	Creep* creep = TOY_VALUE_AS_OPAQUE(value);

	if (creep < player->creeps || creep >= player->creeps + player->creepCapacity) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument value found in DestroyCreep() (This doesn't appear to be a Creep)");
		Toy_error(buffer);
		Toy_freeValue(value);
		return;
	}

	//get the creep's index and mark them as dead
	unsigned int index = creep - player->creeps;
	player->creeps[index].active = false;
	player->creepCount--;

	//nullify the scoped variable
	if (TOY_VALUE_IS_REFERENCE(value)) {
		(*TOY_VALUE_AS_REFERENCE(value)) = TOY_VALUE_FROM_NULL();
	}
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"CreateCreep", api_createCreep},
	{"DestroyCreep", api_destroyCreep},
	{NULL, NULL},
};

void initPlayerAPI(Toy_VM* vm, Player* player) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the creep API, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		//I'm pretty sure this is a cardinal sin
		void* ptr = (void*)(&fn->native.meta1);
		*((Player**)ptr) = player;

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

	//Any cleanup needed goes here
	Toy_VM vm = player->vm; //take ownership away from the player object

	//this will also free the player's allocated memory within the bucket
	Toy_freeVM(&vm);
}
