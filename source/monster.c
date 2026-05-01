#include "monster.h"

#include "toy_console_colors.h"
#include "toy_table.h"
#include "toy_array.h"
#include "toy_string.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>

//sprites loaded from disk
typedef struct MonsterSprite {
	Texture2D texture;
	Rectangle rect;
} MonsterSprite;

//Monsters loaded from scripts
typedef struct MonsterData {
	MonsterSprite* sprite;
	Vector2 position;
	int health;
} MonsterData;

//static storage
static Toy_Table* spriteTable = NULL;
static Toy_Array* monsterArray = NULL;

//callbacks
static void loadMonsterSprite(Toy_VM* vm) {
	//key, file, width, height -> null

	//check parameter count
	if (vm->stack->count < 4) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Not enough parameters found in 'loadMonsterSprite'" TOY_CC_RESET "\n");
		return;
	}

	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);
	Toy_Value file = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_STRING(file) || !TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameter types found in 'loadMonsterSprite'" TOY_CC_RESET "\n");
		return;
	}

	//check for overwriting the key
	if ( TOY_VALUE_IS_NULL(Toy_lookupTable(&spriteTable, key)) != true ) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't overwrite existing monster sprite key" TOY_CC_RESET "\n");
		Toy_freeValue(key);
		Toy_freeValue(file);
		Toy_freeValue(width);
		Toy_freeValue(height);
		return;
	}

	//create the sprite stored in the bucket
	MonsterSprite* sprite = (MonsterSprite*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(MonsterSprite));
	sprite->rect = (Rectangle){ 0, 0, TOY_VALUE_AS_INTEGER(width), TOY_VALUE_AS_INTEGER(height) };

	//load the texture from a file
	Toy_String* str = TOY_VALUE_AS_STRING(file);
	if (str->info.type == TOY_STRING_LEAF) {
		sprite->texture = LoadTexture(str->leaf.data);
	}
	else {
		char* cstr = Toy_getStringRaw(str);
		sprite->texture = LoadTexture(cstr);
		free(cstr);
	}

	//insert into the table as an opaque
	Toy_insertTable(&spriteTable, key, TOY_OPAQUE_FROM_POINTER(sprite));
}

static void spawnMonsterAt(Toy_VM* vm) {
	//sprite, x, y -> void

	//check parameter count
	if (vm->stack->count < 3) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Not enough parameters found in 'spawnMonsterAt'" TOY_CC_RESET "\n");
		return;
	}

	Toy_Value ypos = Toy_popStack(&vm->stack);
	Toy_Value xpos = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_INTEGER(xpos) || !TOY_VALUE_IS_INTEGER(ypos)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameter types found in 'spawnMonsterAt'" TOY_CC_RESET "\n");
		Toy_freeValue(key);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	//get the sprite
	Toy_Value spriteValue = Toy_lookupTable(&spriteTable, key);
	if (TOY_VALUE_IS_NULL(spriteValue)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't spawn a monster with a non-existant sprite" TOY_CC_RESET "\n");
		Toy_freeValue(key);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	//expand the array if needed
	if (monsterArray->count == monsterArray->capacity) {
		monsterArray = Toy_resizeArray(monsterArray, monsterArray->capacity * TOY_ARRAY_EXPANSION_RATE);
		//set the new entries to null values
		for (unsigned int i = monsterArray->count; i < monsterArray->capacity; i++) {
			monsterArray->data[i] = TOY_VALUE_FROM_NULL();
		}
	}

	//find an existing spot for the new monster, overwriting a dead one
	MonsterData* newMonsterPtr = NULL;
	for (unsigned int i = 0; i < monsterArray->count; i++) {
		MonsterData* mData = (MonsterData*)TOY_VALUE_AS_OPAQUE(monsterArray->data[i]);
		if (mData->health <= 0) { //if this monster is dead, steal the slot
			newMonsterPtr = mData;
			break;
		}
	}

	//if no dead monsters were found, make a new slot
	if (newMonsterPtr == NULL) {
		newMonsterPtr = (MonsterData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(MonsterData));
		monsterArray->data[monsterArray->count++] = TOY_OPAQUE_FROM_POINTER(newMonsterPtr);
	}

	//finally, store the new monster's data
	(*newMonsterPtr) = (MonsterData){
		.sprite = (MonsterSprite*)(TOY_VALUE_AS_OPAQUE(spriteValue)),
		.position = { TOY_VALUE_AS_INTEGER(xpos), TOY_VALUE_AS_INTEGER(ypos) },
		.health = 10,
	};
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

CallbackPairs callbackPairs[] = {
	{"loadMonsterSprite", loadMonsterSprite},
	{"spawnMonsterAt", spawnMonsterAt},

	{NULL, NULL},
};

//exposed
void initMonsterObjectPool(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize standard library, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_toString(&(vm->memoryBucket), callbackPairs[i].name);
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		Toy_declareScope(vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}

	//make the local storage (these use malloc(), don't they?)
	spriteTable = Toy_allocateTable(TOY_TABLE_INITIAL_CAPACITY);
	monsterArray = Toy_resizeArray(NULL, TOY_ARRAY_INITIAL_CAPACITY);
}

void freeMonsterObjectPool(Toy_VM* vm) {
	(void)vm;

	Toy_freeTable(spriteTable);
	spriteTable = NULL;
	
	monsterArray = Toy_resizeArray(monsterArray, 0);
}

void drawMonsterPool(Toy_VM* vm) {
	(void)vm;
	for (unsigned int i = 0; i < monsterArray->count; i++) {
		MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(monsterArray->data[i]);

		if (monster->health > 0) {
			DrawTextureRec(monster->sprite->texture, monster->sprite->rect, monster->position, WHITE);
		}
	}
}