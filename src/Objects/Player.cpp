#include "Player.h"

// ============================================================================
// SANTI: TEMPORARY STUB IMPLEMENTATIONS
// ============================================================================
// These functions are declared in Player.h but were missing definitions because
// Player.cpp was empty. They are intentionally minimal and only exist to:
// 1) Unblock compilation/linking.
// 2) Keep the API stable while gameplay logic is ported in Step 6+.
//
// Replace these stubs with real player action logic later.
// ============================================================================

void Player::update(float downTime) {
	// SANTI: Stub. Player movement is currently driven by
	// World::applyFrameMovement() calling Player::applyMoveDirection().
	(void)downTime;
}

void Player::kickBall() {
	// SANTI: Stub. Kick/pass/shot logic will be implemented via Match/World/Ball
	// once you port gameplay from the previous project.
}
