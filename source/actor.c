#include "actor.h"

#include "toy_console_colors.h"
#include "toy_table.h"
#include "toy_array.h"
#include "toy_string.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static Toy_Function* monsterStep = NULL;

//callbacks
static void loadMonsterSprite(Toy_VM* vm) {
	//key, file, width, height -> null

	if (!IsWindowReady()) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't load monster sprites before the window has been initialized" TOY_CC_RESET "\n");
		return;
	}

	//check for initialization
	if (spriteTable == NULL || monsterArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for monster system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

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

	//check for initialization
	if (spriteTable == NULL || monsterArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for monster system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

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
		Toy_String* string = Toy_stringifyValue(&(vm->memoryBucket), key);
		char* cstr = Toy_getStringRaw(string);
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't spawn a monster with a non-existant sprite '%s'" TOY_CC_RESET "\n", cstr);
		free(cstr);
		Toy_freeString(string);
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

static void setMonsterStep(Toy_VM* vm) {
	Toy_Value value = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_FUNCTION(value) && !TOY_VALUE_IS_NULL(value)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad argument type found in 'setMonsterStep', exiting" TOY_CC_RESET "\n");
		exit(-1);
	}

	if (TOY_VALUE_IS_FUNCTION(value)) {
		if (TOY_VALUE_AS_FUNCTION(value)->type != TOY_FUNCTION_CUSTOM) {
			fprintf(stderr, TOY_CC_ERROR "ERROR: Bad function found in 'setMonsterStep', exiting (only allows custom functions or null)" TOY_CC_RESET "\n");
			exit(-1);
		}
		monsterStep = TOY_VALUE_AS_FUNCTION(value);
	}
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"loadMonsterSprite", loadMonsterSprite},
	{"spawnMonsterAt", spawnMonsterAt},
	{"setMonsterStep", setMonsterStep},

	{NULL, NULL},
};

//exposed
void initMonsterAPI(Toy_VM* vm) {
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

void freeMonsterAPI(Toy_VM* vm) {
	(void)vm;

	//free the GL textures
	for (unsigned int i = 0; i < spriteTable->capacity; i++) {
		if (TOY_VALUE_IS_OPAQUE(spriteTable->data[i].value)) {
			UnloadTexture(((MonsterSprite*)TOY_VALUE_AS_OPAQUE(spriteTable->data[i].value))->texture);
		}
	}

	Toy_freeTable(spriteTable);
	spriteTable = NULL;
	
	monsterArray = Toy_resizeArray(monsterArray, 0);
}

void processMonsterStep(Toy_VM* vm) {
	//check for initialization
	if (spriteTable == NULL || monsterArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for monster system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	if (monsterStep == NULL) {
		return; //no-op
	}

	//BUG: invoking a callback with a parameter is process-heavy

	//bind a sub-vm
	Toy_VM subVM;
	Toy_inheritVM(vm, &subVM);
	Toy_bindVM(&subVM, monsterStep->bytecode.code, monsterStep->bytecode.parentScope);

	//paramAddr is relative to the data section, and is followed by the param type
	unsigned int paramAddr = ((unsigned int*)(subVM.code + subVM.paramAddr))[0];
	Toy_ValueType paramType = (Toy_ValueType)(((unsigned int*)(subVM.code + subVM.paramAddr))[1]);

	//c-string of the param's name && as a name string
	const char* cstr = ((char*)(subVM.code + subVM.dataAddr)) + paramAddr;
	Toy_String* name = Toy_toStringLength(&subVM.memoryBucket, cstr, strlen(cstr));

	//load each valid monster and process them one at a time
	for (unsigned int i = 0; i < monsterArray->count; i++) {
		MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(monsterArray->data[i]);
		if (monster->health > 0) {
			subVM.scope = Toy_pushScope(&subVM.memoryBucket, subVM.scope);

			Toy_declareScope(subVM.scope, name, paramType, Toy_copyValue(&subVM.memoryBucket, monsterArray->data[i]), true);
			Toy_runVM(&subVM);

			subVM.scope = Toy_popScope(subVM.scope);
		}
	}

	Toy_freeVM(&subVM);
}

void drawMonsters(Toy_VM* vm) {
	(void)vm;

	//check for initialization
	if (spriteTable == NULL || monsterArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for monster system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	for (unsigned int i = 0; i < monsterArray->count; i++) {
		MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(monsterArray->data[i]);

		if (monster->health > 0) {
			DrawTextureRec(monster->sprite->texture, monster->sprite->rect, monster->position, WHITE);
		}
	}
}

static void attr_monsterSetX(Toy_VM* vm) {
	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(x)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad argument type in MonsterData.setX() (expected 'Int', found '%s')" TOY_CC_RESET "\n", Toy_getValueTypeAsCString(x.type));
	}

	MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(compound);

	monster->position.x = TOY_VALUE_AS_INTEGER(x);
}

static void attr_monsterSetY(Toy_VM* vm) {
	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(y)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad argument type in MonsterData.setY() (expected 'Int', found '%s')" TOY_CC_RESET "\n", Toy_getValueTypeAsCString(y.type));
	}

	MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(compound);

	monster->position.y = TOY_VALUE_AS_INTEGER(y);
}

Toy_Value handleMonsterAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//check for initialization
	if (spriteTable == NULL || monsterArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for monster system hasn't been initialized" TOY_CC_RESET "\n");
		return TOY_VALUE_FROM_NULL();
	}

	//check for correct types
	if (!TOY_VALUE_IS_OPAQUE(compound) || !TOY_VALUE_IS_STRING(attribute) || TOY_VALUE_AS_STRING(attribute)->info.type != TOY_STRING_LEAF) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameters found in 'handleMonsterAttributes'" TOY_CC_RESET "\n");
		return TOY_VALUE_FROM_NULL(); //do not free the params here
	}

	MonsterData* monster = (MonsterData*)TOY_VALUE_AS_OPAQUE(compound);


	if (TOY_VALUE_AS_STRING(attribute)->info.length == 1 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "x", 1)  == 0) {
		return TOY_VALUE_FROM_INTEGER(monster->position.x);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 1 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "y", 1)  == 0) {
		return TOY_VALUE_FROM_INTEGER(monster->position.y);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 4 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "setX", 4)  == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_monsterSetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 4 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "setY", 4)  == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_monsterSetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of type MonsterData (an Opaque)", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}