/**
 * @file Player.cpp
 * @brief Player method implementations.
 */

#include "Player.h"

// ============================================================================
// TEMPORARY MINIMAL IMPLEMENTATIONS
// ============================================================================
// These functions are declared in Player.h but were missing definitions because
// Player.cpp was empty. They are intentionally minimal and only exist to:
// 1) Unblock compilation/linking.
// 2) Keep the API stable while gameplay logic lives in Match/World/Ball.
//
// Replace these stubs with real player action logic later.
// ============================================================================

void Player::update(float downTime) {
	// Stub. Player movement is currently driven by
	// World::applyFrameMovement() calling Player::applyMoveDirection().
	(void)downTime;
}

/**
 * @brief Legacy action hook kept for API compatibility.
 */
void Player::kickBall() {
	// Stub. Kick/pass/shot logic is handled via Match/World/Ball.
}
