#pragma once

#include "toy_vm.h"
#include "creep.h"

typedef struct Player {
	Toy_VM vm;
	unsigned int creepCapacity;
	unsigned int creepCount;
	Creep* creeps;
} Player;

Player* allocatePlayer();
void bindBytecodeToPlayer(Player* player, unsigned char* bytecode);
void freePlayer(Player*);

void initPlayerAPI(Toy_VM* vm);