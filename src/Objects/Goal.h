#pragma once
#include "Ball.h"

class Goal {
public:
	Goal() = default;

	// SANTI: 0 = left goal (home defends), 1 = right goal (away defends)
	void setSide(int side) { mSide = side; }

	bool checkBallCollision(const Ball& ball);

private:
	int mSide = 0; // SANTI: default initialize
};