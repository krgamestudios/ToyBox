#include "aasquare.h"

static AASquare euclid(AASquare square) {
	if (square.min.x > square.max.x) {
		float tmp = square.min.x;
		square.min.x = square.max.x;
		square.max.x = tmp;
	}

	if (square.min.y > square.max.y) {
		float tmp = square.min.y;
		square.min.y = square.max.y;
		square.max.y = tmp;
	}

	return square;
}

AASquare makeAASquare(float x1, float y1, float x2, float y2) {
	AASquare square = {
		.min = {x1, y1},
		.max = {x2, y2},
	};

	return euclid(square);
}

AASquare makeAASquareFromRectangle(Rectangle rect) {
	AASquare square = {
		.min = {rect.x, rect.y},
		.max = {rect.x + rect.width, rect.y + rect.height},
	};

	return euclid(square);
}

bool checkForAASquareWorldOverlap(Vector2 lhsOrigin, AASquare lhs, Vector2 rhsOrigin, AASquare rhs) {
	lhs.min.x += lhsOrigin.x;
	lhs.min.y += lhsOrigin.y;
	lhs.max.x += lhsOrigin.x;
	lhs.max.y += lhsOrigin.y;

	rhs.min.x += rhsOrigin.x;
	rhs.min.y += rhsOrigin.y;
	rhs.max.x += rhsOrigin.x;
	rhs.max.y += rhsOrigin.y;

	return checkForAASquareOverlap(lhs, rhs);
}

bool checkForAASquareOverlap(AASquare lhs, AASquare rhs) {
	lhs = euclid(lhs);
	rhs = euclid(rhs);

	//inverted for short circuiting
	return !(
		lhs.min.x > rhs.max.x ||
		lhs.max.x < rhs.min.x ||
		lhs.min.y > rhs.max.y ||
		lhs.max.y < rhs.min.y
	);
}

void debugDrawAASquare(AASquare square, Color color) {
	DrawRectangleLines(
		square.min.x, square.min.y,
		square.max.x - square.min.x, square.max.y - square.min.y,
		color
	);
}