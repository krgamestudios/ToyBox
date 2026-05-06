#pragma once

#include "toy_vm.h"

//object pool system
void initMonsterAPI(Toy_VM* vm);
void freeMonsterAPI(Toy_VM* vm);

void processMonsterStep(Toy_VM* vm);
void drawMonsters(Toy_VM* vm);

Toy_Value handleMonsterAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);