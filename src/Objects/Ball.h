#pragma once

/**
 * @file Ball.h
 * @brief Ball entity with ownership, velocity, cooldown, and guided travel.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help draft/format documentation
 * comments and to sanity-check the structure of the "guided travel" (time-based travel from
 * start to end) state. The team implemented the behavior and integrated it with World/Match.
 *
 * Example prompt used:
 * "Review this Ball class header. Suggest concise Doxygen comments and highlight any common
 * pitfalls for representing a time-based 'guided travel' state alongside velocity-based motion.
 * Do not change runtime behavior."
 */

#include "SFML/Graphics.hpp"
#include "../Common/Constants.h"

// ============================================================================
// BALL CLASS
// ============================================================================
// Represents the ball on the field. Inherits from sf::CircleShape for rendering.
// Owns possession (owner player ID), steal cooldown, and velocity.
//
// The ball is simulated by the host. Snapshot data is exported via World.
// ============================================================================

/**
 * @class Ball
 * @brief Owns ball physics state and possession state.
 *
 * Ball does not choose pass targets, shot targets, or interceptions. World and
 * MatchState decide those rules; Ball executes movement.
 */
class Ball : public sf::CircleShape
{
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION
	// ------------------------------------------------------------------------
	// Defaults: mOwnerID = -1, mStealCooldown = 0.f, radius = Config::BALL_RADIUS,
	//           fill color = White, origin at center.
	Ball() : mOwnerID(-1), mStealCooldown(0.f) {
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
	/** @brief Apply an explicit velocity vector and clear current owner. */
	void applyKick(sf::Vector2f direction);

	/** @brief Legacy/default pass impulse helper. */
	void applyPass();

	/** @brief Legacy/default shot impulse helper. */
	void applyShot();

	/** @brief Advance cooldowns, guided travel, and velocity-based movement. */
	void update(const float deltaTime);

	/** @brief Kick the ball toward a target point at a requested speed. */
	void kickToward(const sf::Vector2f& target, float speed);

	// ------------------------------------------------------------------------
	// GUIDED TRAVEL
	// ------------------------------------------------------------------------
	// Guaranteed passes/shots are "guided" (lerp along a segment for a computed
	// duration). During guided travel the ball has no
	// owner. At the end, the ball is either:
	// - given to a specified receiver (successful pass / intercepted pass/shot), or
	// - left loose (e.g. shot reaches the goal target).
	//
	// NOTE: The decision of *where* to travel and *who* should receive is NOT
	// Ball's job. Ball only executes the travel and applies the final owner.
	bool isGuidedInFlight() const { return mGuided.active; }

	// Clears any in-flight guided travel immediately. This is required when
	// restarting play so the ball does not continue traveling from an old pass/shot.
	void cancelGuidedTravel() { mGuided = GuidedTravelState{}; }

	// Starts a guided travel from the ball's current position
	// to 'end'. Travel time is derived from distance/speed with a minimum
	// duration. If finalOwnerId >= 0, the ball will be assigned to that player
	// when the travel completes.
	void beginGuidedTravel(const sf::Vector2f& end, float travelSpeed, int finalOwnerId);

	// ------------------------------------------------------------------------
	// VELOCITY GETTER / SETTER
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

	// Guided travel state (pass/shot in flight).
	struct GuidedTravelState {
		bool active = false;
		sf::Vector2f start{ 0.f, 0.f };
		sf::Vector2f end{ 0.f, 0.f };
		float durationSec = 0.f;
		float elapsedSec = 0.f;
		int finalOwnerId = -1; // -1 means "leave ball loose" when travel ends.
	};
	GuidedTravelState mGuided{};
};
