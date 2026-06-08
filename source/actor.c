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
	//file, width, height -> null
	(void)self;

	if (!IsWindowReady()) {
		char buffer[256];
		sprintf(buffer, "Can't load actor sprites before the window has been initialized");
		Toy_error(buffer);
		return;
	}

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		sprintf(buffer, "Object pool for actor system hasn't been initialized before the call to 'loadSprite'");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 3) {
		char buffer[256];
		sprintf(buffer, "Not enough parameters found in 'loadSprite'");
		Toy_error(buffer);
		return;
	}

	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);
	Toy_Value file = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_STRING(file) || !TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		char buffer[256];
		sprintf(buffer, "Bad parameter types found in 'loadSprite'");
		Toy_error(buffer);
		return;
	}

	//look to see if this file is already in memory
	Toy_Value spriteValue = Toy_lookupTable(&spriteTable, file);

	//if not, load it in
	if (TOY_VALUE_IS_NULL(spriteValue)) {
		//create the sprite's data stored in the bucket
		SpriteData* sprite = (SpriteData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(SpriteData));

		sprite->type = OPAQUE_SPRITE_DATA;

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

		sprite->rect = (Rectangle){ 0, 0, TOY_VALUE_AS_INTEGER(width), TOY_VALUE_AS_INTEGER(height) };
		sprite->states = NULL;

		//save the sprite as an opaque
		spriteValue = TOY_OPAQUE_FROM_POINTER(sprite);
		Toy_insertTable(&spriteTable, Toy_copyValue(&vm->memoryBucket, file), spriteValue);
	}

	//return the sprite
	Toy_pushStack(&vm->stack, spriteValue);

	Toy_freeValue(file);
	Toy_freeValue(width);
	Toy_freeValue(height);
}

static void api_spawnActorAt(Toy_VM* vm, Toy_FunctionNative* self) {
	//sprite, onStep, x, y -> Opaque(Actor)
	(void)self;

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		sprintf(buffer, "Object pool for actor system hasn't been initialized before the call to 'spawnActorAt'");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 4) {
		char buffer[256];
		sprintf(buffer, "Not enough parameters found in 'spawnActorAt'");
		Toy_error(buffer);
		return;
	}

	Toy_Value step = Toy_popStack(&vm->stack);
	Toy_Value sprite = Toy_popStack(&vm->stack);
	Toy_Value ypos = Toy_popStack(&vm->stack);
	Toy_Value xpos = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(xpos)) {
		xpos = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(xpos)));
	}
	if (TOY_VALUE_IS_INTEGER(ypos)) {
		ypos = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(ypos)));
	}

	//check types
	if (!TOY_VALUE_IS_FLOAT(xpos) || !TOY_VALUE_IS_FLOAT(ypos) || !TOY_VALUE_IS_OPAQUE(sprite) || !(TOY_VALUE_IS_FUNCTION(step) || TOY_VALUE_IS_NULL(step))) {
		char buffer[256];
		sprintf(buffer, "Bad parameter types found in 'spawnActorAt'");
		Toy_error(buffer);
		Toy_freeValue(sprite);
		Toy_freeValue(step);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	//check the sprite data
	if (*(OpaqueType*)TOY_VALUE_AS_OPAQUE(sprite) != OPAQUE_SPRITE_DATA) {
		char buffer[256];
		sprintf(buffer, "Bad parameter types found in 'spawnActorAt' (expected opaque 'SpriteData')");
		Toy_error(buffer);
		Toy_freeValue(sprite);
		Toy_freeValue(step);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		return;
	}

	//get the function
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
			actorData->spriteData = NULL;
			actorData->spriteState = NULL;
			actorData->currentFrame = 0;
			actorData->position = (Vector2){0,0};
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
		.type = OPAQUE_ACTOR_DATA,
		.spriteData = (SpriteData*)(TOY_VALUE_AS_OPAQUE(sprite)),
		.spriteState = NULL, //TODO: incomplete
		.currentFrame = 0,
		.position = { TOY_VALUE_AS_FLOAT(xpos), TOY_VALUE_AS_FLOAT(ypos) },
		.onStep = onStep,
		.enabled = true,
	};

	//leave the actor on the stack
	Toy_Value value = TOY_OPAQUE_FROM_POINTER(newActorPtr);
	Toy_pushStack(&vm->stack, value);

	Toy_freeValue(xpos);
	Toy_freeValue(ypos);
	Toy_freeValue(sprite);
	Toy_freeValue(step);
}

//callback utils
typedef struct CallbackPairs {
	const char* name;
	Toy_nativeCallback callback;
} CallbackPairs;

static CallbackPairs callbackPairs[] = {
	{"LoadSprite", api_loadSprite},
	// {"unloadSprite", unloadSprite},
	{"SpawnActorAt", api_spawnActorAt},
	// {"despawnActor", despawnActor},

	{NULL, NULL},
};

//exposed
void initActorAPI(Toy_VM* vm) {
	if (vm == NULL || vm->scope == NULL || vm->memoryBucket == NULL) {
		fprintf(stderr, TOY_CC_ERROR "ERROR: Can't initialize the actor API, exiting\n" TOY_CC_RESET);
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
		actorData->spriteData = NULL;
		actorData->spriteState = NULL;
		actorData->currentFrame = 0;
		actorData->position = (Vector2){0,0};
		if (actorData->onStep != NULL) {
			Toy_freeFunction(actorData->onStep);
			actorData->onStep = NULL;
		}
	}

	actorArray = Toy_resizeArray(actorArray, 0);
}

void processActors(Toy_VM* vm) {
	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		sprintf(buffer, "Object pool for actor system hasn't been initialized before the call to 'processActors'");
		Toy_error(buffer);
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
		char buffer[256];
		sprintf(buffer, "Object pool for actor system hasn't been initialized before the call to 'drawActors'");
		Toy_error(buffer);
		return;
	}

	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (actor->enabled) {
			DrawTextureRec(actor->spriteData->texture, actor->spriteData->rect, actor->position, WHITE);

			//TODO: incomplete
			// DrawTexturePro(
			// 	actor->spriteData->texture, actor->spriteData->rect,
			// 	(Rectangle){actor->position.x, actor->position.y, 128, 128},
			// 	(Vector2){0,0}, 0, WHITE
			// );
		}
	}
}

//opaque attributes
static void attr_actorSetX(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value x = Toy_popStack(&vm->stack);

	//type coersion
	if (TOY_VALUE_IS_INTEGER(x)) {
		x = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(x)));
	}

	if (!TOY_VALUE_IS_FLOAT(x)) {
		char buffer[256];
		sprintf(buffer, "Bad argument type in ActorData.setX() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(x.type));
		Toy_error(buffer);
		return;
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);
	actor->position.x = TOY_VALUE_AS_FLOAT(x);
}

static void attr_actorSetY(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value y = Toy_popStack(&vm->stack);

	if (TOY_VALUE_IS_INTEGER(y)) {
		y = TOY_VALUE_FROM_FLOAT((float)(TOY_VALUE_AS_INTEGER(y)));
	}

	if (!TOY_VALUE_IS_FLOAT(y)) {
		char buffer[256];
		sprintf(buffer, "Bad argument type in ActorData.setY() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		return;
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);
	actor->position.y = TOY_VALUE_AS_FLOAT(y);
}

static void attr_actorSetSpriteState(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)vm;
	(void)self;
	//TODO: incomplete
}

#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		sprintf(buffer, "Object pool for actor system hasn't been initialized before the call to 'handleActorAttributes'");
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}

	//useable actor
	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	if (CSTR_MATCH(cstr, "x")) {
		return TOY_VALUE_FROM_INTEGER(actor->position.x);
	}
	else if (CSTR_MATCH(cstr, "y")) {
		return TOY_VALUE_FROM_INTEGER(actor->position.y);
	}
	else if (CSTR_MATCH(cstr, "setX")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetX);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setY")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetY);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "setSpriteState")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetSpriteState);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'ActorData' requested in 'handleActorAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}