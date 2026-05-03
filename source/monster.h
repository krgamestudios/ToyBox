#pragma once

#include "toy_vm.h"

//object pool system
void initMonsterObjectPool(Toy_VM* vm);
void freeMonsterObjectPool(Toy_VM* vm);

void drawMonsters(Toy_VM* vm);
