#pragma once

#include "raylib.h"

typedef struct AASquare {
	Vector2 min;
	Vector2 max;
} AASquare;

AASquare makeAASquare(float x1, float y1, float x2, float y2);
AASquare makeAASquareFromRectangle(Rectangle rect);

bool checkForAASquareWorldOverlap(Vector2 lhsOrigin, AASquare lhs, Vector2 rhsOrigin, AASquare rhs);
bool checkForAASquareOverlap(AASquare lhs, AASquare rhs);

void debugDrawAASquare(AASquare square, Color color);