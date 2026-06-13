#include "actor.h"

#include "toy_console_colors.h"
#include "toy_table.h"
#include "toy_array.h"
#include "toy_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//util macro
#define CSTR_MATCH(FIRST, SECOND) (strlen(FIRST) == strlen(SECOND) && strcmp(FIRST, SECOND) == 0)

//static storage
static Toy_Table* spriteTable = NULL;
static Toy_Array* actorArray = NULL;

//API bindings
static void api_loadSprite(Toy_VM* vm, Toy_FunctionNative* self) {
	//file, width, height -> null
	(void)self;

	if (!IsWindowReady()) {
		char buffer[256];
		snprintf(buffer, 256, "Can't load actor sprites before the window has been initialized");
		Toy_error(buffer);
		return;
	}

	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		snprintf(buffer, 256, "Object pool for actor system hasn't been initialized before the call to 'loadSprite'");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 3) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'loadSprite'");
		Toy_error(buffer);
		return;
	}

	Toy_Value height = Toy_popStack(&vm->stack);
	Toy_Value width = Toy_popStack(&vm->stack);
	Toy_Value file = Toy_popStack(&vm->stack);

	//check types
	if (!TOY_VALUE_IS_STRING(file) || !TOY_VALUE_IS_INTEGER(width) || !TOY_VALUE_IS_INTEGER(height)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'loadSprite'");
		Toy_error(buffer);
		Toy_freeValue(file);
		Toy_freeValue(width);
		Toy_freeValue(height);
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
		snprintf(buffer, 256, "Object pool for actor system hasn't been initialized before the call to 'spawnActorAt'");
		Toy_error(buffer);
		return;
	}

	//check parameter count
	if (vm->stack->count < 4) {
		char buffer[256];
		snprintf(buffer, 256, "Not enough parameters found in 'spawnActorAt'");
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
		snprintf(buffer, 256, "Bad parameter types found in 'spawnActorAt'");
		Toy_error(buffer);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		Toy_freeValue(sprite);
		Toy_freeValue(step);
		return;
	}

	//check the sprite data
	if (*(OpaqueType*)TOY_VALUE_AS_OPAQUE(sprite) != OPAQUE_SPRITE_DATA) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types found in 'spawnActorAt' (expected opaque 'SpriteData')");
		Toy_error(buffer);
		Toy_freeValue(xpos);
		Toy_freeValue(ypos);
		Toy_freeValue(sprite);
		Toy_freeValue(step);
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

	//find an existing spot for the new actor, replacing a null one if able
	Toy_Value* valuePtr = NULL;

	//if an actor has been cleared, steal the slot
	for (unsigned int i = 0; i < actorArray->count; i++) {
		if (TOY_VALUE_IS_NULL(actorArray->data[i])) {
			ActorData* actorData = (ActorData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(ActorData));
			actorArray->data[i] = TOY_OPAQUE_FROM_POINTER(actorData);
			valuePtr = &actorArray->data[i];
			break;
		}
	}

	//if no null slots were found, make a new slot
	if (valuePtr == NULL) {
		ActorData* actorData = (ActorData*)Toy_partitionBucket(&(vm->memoryBucket), sizeof(ActorData));
		actorArray->data[actorArray->count++] = TOY_OPAQUE_FROM_POINTER(actorData);
		valuePtr = &actorArray->data[actorArray->count-1];
	}

	//finally, store the new actor's data
	(*(ActorData*)TOY_VALUE_AS_OPAQUE(*valuePtr)) = (ActorData){
		.type = OPAQUE_ACTOR_DATA,
		.spriteData = (SpriteData*)(TOY_VALUE_AS_OPAQUE(sprite)),
		.spriteState = NULL,
		.currentFrame = 0,
		.position = { TOY_VALUE_AS_FLOAT(xpos), TOY_VALUE_AS_FLOAT(ypos) },
		.onStep = onStep,
	};

	//leave the actor's reference on the stack
	Toy_pushStack(&vm->stack, TOY_REFERENCE_FROM_POINTER(valuePtr));

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

//URGENT: No "unload" is currently implemented (for sprites)
static CallbackPairs callbackPairs[] = {
	{"LoadSprite", api_loadSprite},
	{"SpawnActorAt", api_spawnActorAt},

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
	for (unsigned int i = 0; spriteTable != NULL && i < spriteTable->capacity; i++) {
		if (!TOY_VALUE_IS_NULL(spriteTable->data[i].key) && TOY_VALUE_IS_OPAQUE(spriteTable->data[i].value)) {
			UnloadTexture(((SpriteData*)TOY_VALUE_AS_OPAQUE(spriteTable->data[i].value))->texture);

			//clear the sprites' states, if any
			Toy_Table* statesTable = ((SpriteData*)TOY_VALUE_AS_OPAQUE(spriteTable->data[i].value))->states;
			for (unsigned int j = 0; statesTable != NULL && j < statesTable->capacity; j++) {
				if (!TOY_VALUE_IS_NULL(statesTable->data[j].key) && TOY_VALUE_IS_OPAQUE(statesTable->data[j].value)) {
					SpriteState* state = (SpriteState*)TOY_VALUE_AS_OPAQUE(statesTable->data[j].value);
					Toy_releaseBucketPartition((unsigned char*)state); //this was partitioned in 'attr_spriteAddAnimationState'
				}
			}

			Toy_freeTable(statesTable);
		}
	}

	Toy_freeTable(spriteTable);
	spriteTable = NULL;

	//unload each actor's members
	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actorData = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (actorData == NULL) {
			continue;
		}

		//free the dead actors' internals
		if (actorData->onStep != NULL) {
			Toy_freeFunction(actorData->onStep);
		}
		Toy_releaseBucketPartition((void*)actorData);
	}

	actorArray = Toy_resizeArray(actorArray, 0);
}

void processActors(Toy_VM* vm) {
	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		snprintf(buffer, 256, "Object pool for actor system hasn't been initialized before the call to 'processActors'");
		Toy_error(buffer);
		return;
	}

	//bind a sub-vm
	Toy_VM subVM;
	Toy_inheritVM(vm, &subVM);

	//load each valid actor and process them one at a time
	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (actor == NULL || actor->onStep == NULL) {
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
		snprintf(buffer, 256, "Object pool for actor system hasn't been initialized before the call to 'drawActors'");
		Toy_error(buffer);
		return;
	}

	for (unsigned int i = 0; i < actorArray->count; i++) {
		ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(actorArray->data[i]);

		if (actor == NULL) {
			continue;
		}

		if (actor->spriteState == NULL) {
			//Texture2D texture, Rectangle source, Vector2 position, Color tint
			DrawTextureRec(actor->spriteData->texture, actor->spriteData->rect, actor->position, WHITE);
		}

		else {
			//Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint
			DrawTexturePro(
				actor->spriteData->texture,
				(Rectangle){
					actor->spriteData->rect.x + actor->currentFrame * actor->spriteData->rect.width,
					actor->spriteData->rect.y + actor->spriteState->stripIndex * actor->spriteData->rect.height,
					actor->spriteData->rect.width,
					actor->spriteData->rect.height,
				},
				(Rectangle){actor->position.x, actor->position.y, actor->spriteData->rect.width, actor->spriteData->rect.height},
				(Vector2){0,0},
				0,
				WHITE
			);

			//increment the currentFrame
			actor->currentFrame = (actor->currentFrame + 1) % actor->spriteState->frameCount;
		}
	}
}

//opaque attributes for sprites
static void attr_spriteAddAnimationState(Toy_VM* vm, Toy_FunctionNative* self) {
	//sprite, anim-key, strip index, frame count
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value frameCount = Toy_popStack(&vm->stack);
	Toy_Value stripIndex = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_STRING(key) || !TOY_VALUE_IS_INTEGER(stripIndex) || !TOY_VALUE_IS_INTEGER(frameCount)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad parameter types in SpriteData.addAnimationState()");
		Toy_error(buffer);
		return;
	}

	//get the sprite object, with a valid state table
	SpriteData* sprite = (SpriteData*)TOY_VALUE_AS_OPAQUE(compound);
	if (sprite->states == NULL) {
		sprite->states = Toy_allocateTable(TOY_TABLE_INITIAL_CAPACITY);
	}

	//check for existing state names
	if (Toy_private_lookupTableEntryPtr(&sprite->states, key) != NULL) {
		char buffer[256];
		snprintf(buffer, 256, "Can't overwrite an existing sprite animation state in SpriteData.addAnimationState()");
		Toy_error(buffer);
		return;
	}

	//make the new state object
	SpriteState* state = (SpriteState*)Toy_partitionBucket(&vm->memoryBucket, sizeof(SpriteState));

	state->stripIndex = TOY_VALUE_AS_INTEGER(stripIndex);
	state->frameCount = TOY_VALUE_AS_INTEGER(frameCount);

	//insert the new state
	Toy_insertTable(&sprite->states, key, TOY_OPAQUE_FROM_POINTER(state));
}

Toy_Value handleSpriteAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	(void)compound;

	//check for initialization
	if (spriteTable == NULL) {
		char buffer[256];
		snprintf(buffer, 256, "Object pool for sprite system hasn't been initialized before the call to 'handleSpriteAttributes'");
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}

	//useable actor
	// SpriteData* sprite = (SpriteData*)TOY_VALUE_AS_OPAQUE(compound);

	//the attribute we're looking for
	Toy_String* string = TOY_VALUE_AS_STRING(attribute);
	const char* cstr = string->leaf.data;

	if (CSTR_MATCH(cstr, "addAnimationState")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_spriteAddAnimationState);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'SpriteData' requested in 'handleSpriteAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}

//opaque attributes for actor
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
		snprintf(buffer, 256, "Bad argument type in ActorData.setX() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(x.type));
		Toy_error(buffer);
		return;
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);
	actor->position.x = TOY_VALUE_AS_FLOAT(x);

	Toy_freeValue(compound);
	Toy_freeValue(x);
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
		snprintf(buffer, 256, "Bad argument type in ActorData.setY() (expected 'Int' or 'Float', found '%s')", Toy_getValueTypeAsCString(y.type));
		Toy_error(buffer);
		return;
	}

	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);
	actor->position.y = TOY_VALUE_AS_FLOAT(y);

	Toy_freeValue(compound);
	Toy_freeValue(y);
}

static void attr_actorSetAnimationState(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack);
	Toy_Value key = Toy_popStack(&vm->stack);

	if (!TOY_VALUE_IS_STRING(key)) {
		char buffer[256];
		snprintf(buffer, 256, "Bad argument type in ActorData.setSpriteState() (expected 'String', found '%s')", Toy_getValueTypeAsCString(key.type));
		Toy_error(buffer);
		Toy_freeValue(compound);
		Toy_freeValue(key);
		return;
	}

	//get actor, sprite & state table
	ActorData* actor = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);
	SpriteData* sprite = actor->spriteData;

	if (sprite->states == NULL) {
		char buffer[256];
		snprintf(buffer, 256, "No sprite states found in Actor's 'SpriteData'");
		Toy_error(buffer);
		Toy_freeValue(compound);
		Toy_freeValue(key);
		return;
	}

	//get the state
	Toy_Value stateValue = Toy_lookupTable(&sprite->states, key);

	if (!TOY_VALUE_IS_OPAQUE(stateValue)) {
		const char* cstr;
		if (TOY_VALUE_AS_STRING(key)->info.type != TOY_STRING_LEAF) {
			cstr = Toy_getStringRaw(TOY_VALUE_AS_STRING(key));
		}
		else {
			cstr = TOY_VALUE_AS_STRING(key)->leaf.data;
		}

		char buffer[256];
		snprintf(buffer, 256, "No sprite state named '%s' found in Actor's 'SpriteData'", cstr);
		Toy_error(buffer);

		if (TOY_VALUE_AS_STRING(key)->info.type != TOY_STRING_LEAF) {
			free((void*)cstr);
		}
		Toy_freeValue(compound);
		Toy_freeValue(key);
		return;
	}

	//finally, set the state and reset the current frame, but ONLY if the state has actually been changed
	if (actor->spriteState != (SpriteState*)TOY_VALUE_AS_OPAQUE(stateValue)) {
		actor->spriteState = (SpriteState*)TOY_VALUE_AS_OPAQUE(stateValue);
		actor->currentFrame = 0;
	}

	Toy_freeValue(compound);
	Toy_freeValue(key);
}

static void attr_actorDespawn(Toy_VM* vm, Toy_FunctionNative* self) {
	(void)self;

	Toy_Value compound = Toy_popStack(&vm->stack); //compound is (presumably) a reference

	if (!TOY_VALUE_IS_REFERENCE(compound)) {
		char buffer[256];
		snprintf(buffer, 256, "Actor can't be despawned in 'ActorData.despawn()', expected value type %s, found %s", Toy_getValueTypeAsCString(TOY_VALUE_REFERENCE), Toy_getValueTypeAsCString(compound.type));
		Toy_error(buffer);
		return;
	}

	//clear the actor data within the bucket
	ActorData* actorData = (ActorData*)TOY_VALUE_AS_OPAQUE(compound);

	if (actorData->onStep != NULL) {
		Toy_freeFunction(actorData->onStep);
	}
	Toy_releaseBucketPartition((void*)actorData);

	//clear the actor data within the static array
	for (unsigned int i = 0; i < actorArray->count; i++) {
		if (TOY_VALUE_AS_OPAQUE(compound) == TOY_VALUE_AS_OPAQUE(actorArray->data[i])) {
			actorArray->data[i] = TOY_VALUE_FROM_NULL();
			return;
		}
	}

	//WARN: the variable name will be invalid from this point onwards, I think?
}

Toy_Value handleActorAttributes(Toy_VM* vm, Toy_Value compound, Toy_Value attribute) {
	//check for initialization
	if (spriteTable == NULL || actorArray == NULL) {
		char buffer[256];
		snprintf(buffer, 256, "Object pool for actor system hasn't been initialized before the call to 'handleActorAttributes'");
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
	else if (CSTR_MATCH(cstr, "setAnimationState")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorSetAnimationState);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}
	else if (CSTR_MATCH(cstr, "despawn")) {
		Toy_Function* fn = Toy_createFunctionFromCallback(&vm->memoryBucket, attr_actorDespawn);
		return TOY_VALUE_FROM_FUNCTION(fn);
	}

	else {
		char buffer[256];
		snprintf(buffer, 256, "Unknown attribute '%s' of 'ActorData' requested in 'handleActorAttributes'", TOY_VALUE_AS_STRING(attribute)->leaf.data);
		Toy_error(buffer);
		return TOY_VALUE_FROM_NULL();
	}
}