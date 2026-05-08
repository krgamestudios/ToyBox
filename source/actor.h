#pragma once

#include "toy_vm.h"

//object pool system
void initActorAPI(Toy_VM* vm);
void freeActorAPI(Toy_VM* vm);

void processActorStep(Toy_VM* vm);
void drawActors(Toy_VM* vm);

Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute);