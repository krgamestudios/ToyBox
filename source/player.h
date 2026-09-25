#pragma once

#include "toy_vm.h"
#include "core.h"
#include "creep.h"

typedef struct Player {
	//game data
	Toy_VM vm;
	Core core;
	Creep* creeps;
	unsigned int creepCapacity;
	unsigned int creepCount;

	//metadata
	int rngSeed;
} Player;

Player* allocatePlayer(int index);
void bindBytecodeToPlayer(Player* player, unsigned char* bytecode);
void freePlayer(Player*);

void initPlayerAPI(Toy_VM* vm, Player* player);