#include "player.h"

#include <stdlib.h>

Player* allocatePlayer() {
    Toy_VM vm;
    Toy_initVM(&vm);
    Player* player = (Player*)Toy_partitionBucket(&vm.memoryBucket, sizeof(Player));
    player->vm = vm;
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

    //this will also free the player's allocated memory within the bucket
    Toy_freeVM(&player->vm);
}