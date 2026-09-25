#pragma once

#include "toy_vm.h"

void initStandardLibrary(Toy_VM*);

//exposed callbacks for engine code
#define RAND_NAME "rand"
void std_rand(Toy_VM* vm, Toy_FunctionNative* self);
void std_srand(Toy_VM* vm, Toy_FunctionNative* self);
void std_time(Toy_VM* vm, Toy_FunctionNative* self);

//NOTE: This build of the standard library has been tweaked to remove 'rand()' and 'srand()' from the player code