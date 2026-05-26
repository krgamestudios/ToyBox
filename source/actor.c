#include "actor.h"

#include "toy_console_colors.h"
#include "toy_table.h"
#include "toy_array.h"
#include "toy_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//static storage
static Toy_Table* spriteTable = NULL;
static Toy_Array* actorArray = NULL;

//API bindings
static void api_loadSprite(Toy_VM* vm, Toy_FunctionNative* self) {
	//key, file, width, height -> null
	(void)self;

	if (!IsWindowReady()) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't load actor sprites before the window has been initialized" TOY_CC_RESET "\n");
		return;
	}

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for actor system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	//check parameter count
	if (vm->stack->count < 4) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Not enough parameters found in 'loadSprite'" TOY_CC_RESET "\n");
		return;
	}

	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);
	Toy_Value file = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_STRING(file) || !TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameter types found in 'loadSprite'" TOY_CC_RESET "\n");
		return;
	}

	//check for overwriting the key
	if ( TOY_VALUE_IS_NULL(Toy_lookupTable(&spriteTable, key)) != true ) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't overwrite existing actor sprite key" TOY_CC_RESET "\n");
		Toy_freeValue(key);
		Toy_freeValue(file);
		Toy_freeValue(width);
		Toy_freeValue(height);
		return;
	}

	//create the sprite stored in the bucket
	SpriteData* sprite = (SpriteData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(SpriteData));
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
	Toy_insertTable(&spriteTable, Toy_copyValue(&vm->memoryBucket, key), TOY_OPAQUE_FROM_POINTER(sprite));

	Toy_freeValue(key);
	Toy_freeValue(file);
	Toy_freeValue(width);
	Toy_freeValue(height);
}

static void api_spawnActorAt(Toy_VM* vm, Toy_FunctionNative* self) {
	//sprite, onStep, x, y -> void
	(void)self;

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for actor system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	//check parameter count
	if (vm->stack->count < 4) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Not enough parameters found in 'spawnActorAt'" TOY_CC_RESET "\n");
		return;
	}

	Toy_Value ypos = Toy_popStack(&vm->stack);
	Toy_Value xpos = Toy_popStack(&vm->stack);
	Toy_Value step = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_INTEGER(xpos) || !TOY_VALUE_IS_INTEGER(ypos) || !(TOY_VALUE_IS_FUNCTION(step) || TOY_VALUE_IS_NULL(step))) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad parameter types found in 'spawnActorAt'" TOY_CC_RESET "\n");
		Toy_freeValue(key);
		Toy_freeValue(step);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	//get the sprite
	Toy_Value spriteValue = Toy_lookupTable(&spriteTable, key);
	if (TOY_VALUE_IS_NULL(spriteValue)) {
		Toy_String* string = Toy_stringifyValue(&(vm->memoryBucket), key);
		char* cstr = Toy_getStringRaw(string);
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't spawn a actor with a non-existant sprite '%s'" TOY_CC_RESET "\n", cstr);
		free(cstr);
		Toy_freeString(string);
		Toy_freeValue(key);
		Toy_freeValue(step);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	Toy_Function* onStep = NULL;
	if (TOY_VALUE_IS_FUNCTION(step)) {
		onStep = Toy_copyFunction(&vm->memoryBucket, TOY_VALUE_AS_FUNCTION(step));
	}

	//expand the array if needed
	if (actorArray->count == actorArray->capacity) {
		actorArray = Toy_resizeArray(actorArray, actorArray->capacity * TOY_ARRAY_EXPANSION_RATE);
		//set the new entries to null values
		for (unsigned int i = actorArray->count; i < actorArray->capacity; i++) {
			actorArray->data[i] = TOY_VALUE_FROM_NULL();
		}
	}

	//find an existing spot for the new actor, overwriting a dead one if able
	ActorData* newActorPtr = NULL;
	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actorData = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);
		if (!actorData->enabled) { //if this actor is dead, steal the slot
			//free the dead actor's internals
			actorData->sprite = NULL;
			if (actorData->onStep != NULL) {
				Toy_freeFunction(actorData->onStep);
				actorData->onStep = NULL;
			}

			newActorPtr = actorData;
			break;
		}
	}

	//if no dead actors were found, make a new slot
	if (newActorPtr == NULL) {
		newActorPtr = (ActorData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(ActorData));
		actorArray->data[actorArray->count++] = TOY_OPAQUE_FROM_POINTER(newActorPtr);
	}

	//finally, store the new actor's data
	(*newActorPtr) = (ActorData){
		.type = OPAQUE_ACTOR,
		.sprite = (SpriteData*)(TOY_VALUE_AS_OPAQUE(spriteValue)),
		.onStep = onStep,
		.position = { TOY_VALUE_AS_INTEGER(xpos), TOY_VALUE_AS_INTEGER(ypos) },
		.enabled = true,
	};

	Toy_freeValue(spriteValue);
	Toy_freeValue(key);
	Toy_freeValue(step);
	Toy_freeValue(xpos);
	Toy_freeValue(ypos);
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"loadSprite", api_loadSprite},
	// {"unloadSprite", unloadSprite},
	{"spawnActorAt", api_spawnActorAt},
	// {"despawnActor", despawnActor},

	{NULL, NULL},
};

//exposed
void initActorAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize standard library, exiting\n" TOY_CC_RESET);
		exit(-1);
	}

	//declare each callback in the global scope
	for (int i = 0; callbackPairs[i].name; i++) {
		Toy_String* key = Toy_createStringLength(&(vm->memoryBucket), callbackPairs[i].name, strlen(callbackPairs[i].name));
		Toy_Function* fn = Toy_createFunctionFromCallback(&(vm->memoryBucket), callbackPairs[i].callback);

		Toy_declareScope(vm->scope, key, TOY_VALUE_FUNCTION, TOY_VALUE_FROM_FUNCTION(fn), true);

		Toy_freeString(key);
	}

	//make the local storage (these use malloc(), don't they?)
	spriteTable = Toy_allocateTable(TOY_TABLE_INITIAL_CAPACITY);
	actorArray = Toy_resizeArray(NULL, TOY_ARRAY_INITIAL_CAPACITY);
}

void freeActorAPI(Toy_VM* vm) {
	(void)vm;

	//free the GL textures
	for (unsigned int i = 0; i < spriteTable->capacity; i++) {
		if (TOY_VALUE_IS_OPAQUE(spriteTable->data[i].value)) {
			UnloadTexture(((SpriteData*)TOY_VALUE_AS_OPAQUE(spriteTable->data[i].value))->texture);
		}
	}

	Toy_freeTable(spriteTable);
	spriteTable = NULL;

	//unload each actor's members
	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actorData = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		//free the dead actor's internals
		actorData->sprite = NULL;
		if (actorData->onStep != NULL) {
			Toy_freeFunction(actorData->onStep);
			actorData->onStep = NULL;
		}
		actorData->position = (Vector2){0,0};
		actorData->enabled = false;
	}

	actorArray = Toy_resizeArray(actorArray, 0);
}

void processActors(Toy_VM* vm) {
	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for actor system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	//bind a sub-vm
	Toy_VM subVM;
	Toy_inheritVM(vm, &subVM);

	//load each valid actor and process them one at a time
	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (!actor->enabled || actor->onStep == NULL) {
			continue;
		}

		//prep the callback
		Toy_bindVM(&subVM, actor->onStep->bytecode.code, actor->onStep->bytecode.parentScope);

		//paramAddr is relative to the data section, and is followed by the param type
		unsigned int paramAddr = ((unsigned int*)(subVM.code + subVM.paramAddr))[0];
		Toy_ValueType paramType = (Toy_ValueType)(((unsigned int*)(subVM.code + subVM.paramAddr))[1]);

		//c-string of the param's name && as a name string
		const char* cstr = ((char*)(subVM.code + subVM.dataAddr)) + paramAddr;
		Toy_String* name = Toy_toStringLength(&subVM.memoryBucket, cstr, strlen(cstr)); //don't use 'create'


		//inject this actor as a parameter
		Toy_declareScope(subVM.scope, name, paramType, Toy_copyValue(&subVM.memoryBucket, actorArray->data[i]), true);

		//run
		Toy_runVM(&subVM);

		//prep the next one
		Toy_resetVM(&subVM, false, true); //preserving the stack prevents memory issues
		subVM.scope = NULL; //BUGFIX: don't retain scope for different callbacks
		Toy_freeString(name);
	}

	Toy_freeVM(&subVM);
}

void drawActors(Toy_VM* vm) {
	(void)vm;

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for actor system hasn't been initialized" TOY_CC_RESET "\n");
		return;
	}

	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (actor->enabled) {
			DrawTextureRec(actor->sprite->texture, actor->sprite->rect, actor->position, WHITE);
		}
	}
}

//opaque bindings
static void attr_actorSetX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(x)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad argument type in ActorData.setX() (expected 'Int', found '%s')" TOY_CC_RESET "\n", Toy_getValueTypeAsCString(x.type));
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);

	actor->position.x = TOY_VALUE_AS_INTEGER(x);
}

static void attr_actorSetY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_INTEGER(y)) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Bad argument type in ActorData.setY() (expected 'Int', found '%s')" TOY_CC_RESET "\n", Toy_getValueTypeAsCString(y.type));
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);

	actor->position.y = TOY_VALUE_AS_INTEGER(y);
}

Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Object pool for actor system hasn't been initialized" TOY_CC_RESET "\n");
		return TOY_VALUE_FROM_NULL();
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);

	if (TOY_VALUE_AS_STRING(attribute)->info.length == 1 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "x", 1)  == 0) {
		return TOY_VALUE_FROM_INTEGER(actor->position.x);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 1 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "y", 1)  == 0) {
		return TOY_VALUE_FROM_INTEGER(actor->position.y);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 4 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "setX", 4)  == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (TOY_VALUE_AS_STRING(attribute)->info.length == 4 && strncmp(TOY_VALUE_AS_STRING(attribute)->leaf.data, "setY", 4)  == 0) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'ActorData'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}