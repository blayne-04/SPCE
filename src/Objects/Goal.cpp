
#include "Goal.h"
#include "../Common/Constants.h"

// SANTI: Manual rect overlap avoids SFML2 vs SFML3 API differences.
static bool rectsOverlap(const sf::FloatRect& a, const sf::FloatRect& b) {
	const float aLeft = a.position.x;
	const float aTop = a.position.y;
	const float aRight = a.position.x + a.size.x;
	const float aBottom = a.position.y + a.size.y;

	const float bLeft = b.position.x;
	const float bTop = b.position.y;
	const float bRight = b.position.x + b.size.x;
	const float bBottom = b.position.y + b.size.y;

	const bool overlapX = (aLeft < bRight) && (aRight > bLeft);
	if (!overlapX) return false;

	const bool overlapY = (aTop < bBottom) && (aBottom > bTop);
	if (!overlapY) return false;

	return true;
}

bool Goal::checkBallCollision(const Ball& ball) {
	const float goalX = (mSide == Config::HOME_TEAM_SIDE) ? Config::LEFT_GOAL_X : Config::RIGHT_GOAL_X;

	const sf::FloatRect goalRect(
		sf::Vector2f(goalX, Config::GOAL_Y_TOP),
		sf::Vector2f(Config::GOAL_WIDTH, Config::GOAL_HEIGHT)
	);

	const sf::FloatRect ballRect = ball.getGlobalBounds();
	return rectsOverlap(ballRect, goalRect);
}
