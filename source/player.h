#pragma once

#include "toy_vm.h"

typedef struct Player {
	Toy_VM vm;
} Player;

Player* allocatePlayer();
void bindBytecodeToPlayer(Player* player, unsigned char* bytecode);
void freePlayer(Player*);