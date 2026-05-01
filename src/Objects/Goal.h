#pragma once

/**
 * @file Goal.h
 * @brief Goal object used for ball scoring collision checks.
 */

#include "Ball.h"

/**
 * @class Goal
 * @brief Represents one goal mouth and detects whether the ball enters it.
 */
class Goal {
public:
	Goal() = default;

	/**
	 * @brief Set which side of the pitch this goal belongs to.
	 * @param side 0 = left goal, 1 = right goal.
	 */
	void setSide(int side) { mSide = side; }

	/**
	 * @brief Check whether the ball overlaps this goal area.
	 * @param ball Ball to test.
	 * @return true if the ball is inside/overlapping the goal mouth.
	 */
	bool checkBallCollision(const Ball& ball);

private:
	int mSide = 0; // default initialize
};
