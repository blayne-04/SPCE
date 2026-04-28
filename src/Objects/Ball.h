#pragma once

#include "SFML/Graphics.hpp"
#include "../Common/Constants.h"

// ============================================================================
// BALL CLASS
// ============================================================================
// Represents the ball on the field. Inherits from sf::CircleShape for rendering.
// Owns possession (owner player ID), steal cooldown, and velocity.
//
// SANTI: Step 4 - added velocity getter/setter and default initializers
//        to support World::writeRawState snapshot and prevent garbage data.
// ============================================================================

class Ball : public sf::CircleShape
{
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION
	// ------------------------------------------------------------------------
	// Defaults: mOwnerID = -1, mStealCooldown = 0.f, radius = Config::BALL_RADIUS,
	//           fill color = White, origin at center.
	Ball() : mOwnerID(-1), mStealCooldown(0.f) {
		// SANTI: Changed hardcoded BALL_RADIUS to Config::BALL_RADIUS.
		setRadius(Config::BALL_RADIUS);
		setFillColor(sf::Color::White);
		setOrigin(sf::Vector2f(getRadius(), getRadius()));
	}

	~Ball() = default;

	// ------------------------------------------------------------------------
	// POSSESSION MANAGEMENT
	// ------------------------------------------------------------------------
	// Sets the owner of the ball (player ID, 0-7, or -1 for no owner).
	void setOwner(const int playerID) { mOwnerID = playerID; }

	// Gets the current owner ID (-1 if loose).
	int getOwner() const { return mOwnerID; }

	// Clears the owner (sets to -1) - used when ball is in flight or after goal.
	void clearOwner() { mOwnerID = -1; }

	// ------------------------------------------------------------------------
	// STEAL COOLDOWN
	// ------------------------------------------------------------------------
	// Returns the remaining cooldown time (seconds) before the ball can be stolen again.
	float getStealCooldown() const { return mStealCooldown; }

	// Sets the steal cooldown timer (typically Config::BALL_STEAL_COOLDOWN_SECONDS).
	void setStealCooldown(const float cooldown) { mStealCooldown = cooldown; }

	// ------------------------------------------------------------------------
	// PHYSICS & ACTIONS (to be implemented in .cpp)
	// ------------------------------------------------------------------------
	void applyKick(sf::Vector2f direction);
	void applyPass();
	void applyShot();
	void update(const float deltaTime);

	// ------------------------------------------------------------------------
	// VELOCITY GETTER / SETTER (SANTI: added for snapshot)
	// ------------------------------------------------------------------------
	sf::Vector2f getVelocity() const { return mVelocity; }
	void setVelocity(sf::Vector2f newVelocity) { mVelocity = newVelocity; }

private:
	// ------------------------------------------------------------------------
	// MEMBER VARIABLES (verbose names, all default-initialized)
	// ------------------------------------------------------------------------
	float mStealCooldown = 0.f;      // Remaining time before next steal attempt (seconds).
	int mOwnerID = -1;               // Player ID (0-7) currently possessing the ball, -1 = loose.
	sf::Vector2f mVelocity{ 0.f, 0.f };   // Current velocity (units per second).
};

// ============================================================================
// SUMMARY OF SANTI CHANGES (Step 4)
// ============================================================================
// 1) Added getVelocity() const and setVelocity(sf::Vector2f) so World can fill
//    GameStatePacket::ballVelocity from the ball object.
// 2) Added default initializers for mStealCooldown (0.f), mOwnerID (-1), and
//    mVelocity (0,0) to prevent reading uninitialized garbage in early MVP frames.
// 3) Changed hardcoded BALL_RADIUS to Config::BALL_RADIUS (already present).
// ============================================================================
