#include "Ball.h"

// ============================================================================
// SANTI: TEMPORARY STUB IMPLEMENTATIONS
// ============================================================================
// These functions are declared in Ball.h but were missing definitions because
// Ball.cpp was empty. They are intentionally minimal and only exist to:
// 1) Unblock compilation/linking.
// 2) Keep the API stable while gameplay logic is ported in Step 6+.
//
// Replace these stubs with real pass/shot/kick logic later.
// ============================================================================

void Ball::applyKick(sf::Vector2f direction) {
	// SANTI: Minimal behavior: ball becomes loose and gets velocity.
	clearOwner();
	setVelocity(direction);
}

void Ball::applyPass() {
	// SANTI: Stub. Real passing will set velocity toward a target and clear owner.
}

void Ball::applyShot() {
	// SANTI: Stub. Real shooting will set velocity toward goal and clear owner.
}

void Ball::update(const float deltaTime) {
	// SANTI: Minimal physics integration so the ball can move if you start
	// calling Ball::update later.
	if (deltaTime <= 0.f) return;

	const sf::Vector2f pos = getPosition() + (mVelocity * deltaTime);
	setPosition(pos);

	// SANTI: Very light damping to avoid perpetual motion in early MVP.
	mVelocity *= Config::BALL_FRICTION;
}
