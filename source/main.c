#include "toy_console_colors.h"

#include "raylib.h"

#include "toy_lexer.h"
#include "toy_parser.h"
#include "toy_compiler.h"
#include "toy_vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//utils
unsigned char* readFile(char* path, int* size) {
	//open the file
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		*size = -1; //missing file error
		return NULL;
	}

	//determine the file's length
	fseek(file, 0L, SEEK_END);
	*size = ftell(file);
	rewind(file);

	//make some space
	unsigned char* buffer = malloc(*size + 1);
	if (buffer == NULL) {
		fclose(file);
		return NULL;
	}

	//read the file
	if (fread(buffer, sizeof(unsigned char), *size, file) < (unsigned int)(*size)) {
		fclose(file);
		*size = -2; //singal a read error
		return NULL;
	}

	buffer[(*size)++] = '\0';

	//clean up and return
	fclose(file);
	return buffer;
}

unsigned char* makeCodeFromSource(const char* source) {
	Toy_Lexer lexer;
	Toy_bindLexer(&lexer, source);

	Toy_Parser parser;
	Toy_bindParser(&parser, &lexer);

	Toy_Bucket* bucket = Toy_allocateBucket(TOY_BUCKET_IDEAL);

	Toy_Ast* ast = Toy_scanParser(&bucket, &parser);
	unsigned char* code = Toy_compileToBytecode(ast);

	Toy_freeBucket(&bucket);

	return code;
}

//main file
int main() {
	//example Toy controlling the window stuff
	int size = 0;
	const char* source = (char*)readFile("config.toy", &size);

	if (!source) {
		fprintf(stderr, "File read error: %d\n", size);
		return -1;
	}

	unsigned char* code = makeCodeFromSource(source);

	Toy_VM vm;
	Toy_initVM(&vm);
	Toy_bindVM(&vm, code, NULL);
	Toy_runVM(&vm);

	//extract the settings
	Toy_Value* screenWidthPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenWidth") );
	Toy_Value* screenHeightPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenHeight") );
	Toy_Value* screenCaptionPtr = Toy_accessScopeAsPointer(vm.scope, Toy_toString(&vm.memoryBucket, "screenCaption") );

	int screenWidth = screenWidthPtr != NULL && TOY_VALUE_IS_INTEGER(*screenWidthPtr) ? TOY_VALUE_AS_INTEGER(*screenWidthPtr) : 640;
	int screenHeight = screenHeightPtr != NULL && TOY_VALUE_IS_INTEGER(*screenHeightPtr) ? TOY_VALUE_AS_INTEGER(*screenHeightPtr) : 480;
	const char* screenCaption = screenCaptionPtr != NULL && TOY_VALUE_IS_STRING(*screenCaptionPtr) ? TOY_VALUE_AS_STRING(*screenCaptionPtr)->leaf.data : "";

	InitWindow(screenWidth, screenHeight, screenCaption);
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		//drawing
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("Do you have games on your phone?", 100, 100, 20, LIGHTGRAY);
		EndDrawing();

	}

	CloseWindow();

	Toy_freeVM(&vm);
	free(code);

	return 0;
}
