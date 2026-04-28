#pragma once

#include "SFML/Graphics.hpp"

// ============================================================================
// PLAYER CLASS
// ============================================================================
// Represents a single player on the field (outfield or goalkeeper).
// Inherits from sf::CircleShape for rendering.
// Owns physics state (position inherited from sf::CircleShape, plus velocity,
// facing direction, lunging flag, team ID, player ID, goalkeeper flag).
//
// SANTI: Step 4 - added init() and const getters to support World::writeRawState.
// ============================================================================

class Player : public sf::CircleShape {
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION
	// ------------------------------------------------------------------------
	Player() = default;

	// SANTI: Step 4 helper to avoid relying on assignment/copy.
	// Initializes all member variables to deterministic values.
	void init(int playerID, int teamID, bool isGoalkeeper) {
		mPlayerID = playerID;
		mTeam = teamID;
		mIsGoalkeeper = isGoalkeeper;

		// Snapshot-safe defaults (prevent uninitialized data).
		mIsLunging = false;
		mVelocity = sf::Vector2f(0.f, 0.f);
		mFacingDirection = sf::Vector2f(1.f, 0.f);   // Facing right initially.
	}

	// ------------------------------------------------------------------------
	// PUBLIC INTERFACE (not yet implemented fully)
	// ------------------------------------------------------------------------
	void update(float downTime);   // TODO: apply input and physics.
	void kickBall();               // TODO: implement kicking logic.

	// ------------------------------------------------------------------------
	// CONST GETTERS (used by World::writeRawState snapshot)
	// ------------------------------------------------------------------------
	int getPlayerID() const { return mPlayerID; }
	int getTeam() const { return mTeam; }

	sf::Vector2f getVelocity() const { return mVelocity; }
	sf::Vector2f getFacingDirection() const { return mFacingDirection; }

	bool IsGoalkeeper() const { return mIsGoalkeeper; }
	bool IsLunging() const { return mIsLunging; }

private:
	// ------------------------------------------------------------------------
	// MEMBER VARIABLES (verbose names, all default-initialized)
	// ------------------------------------------------------------------------

	// SANTI: defaults prevent uninitialized snapshot data.
	int mPlayerID = -1;               // 0-7 (0-3 Home, 4-7 Away)
	int mTeam = 0;                    // 0 = Home, 1 = Away

	sf::Vector2f mVelocity{ 0.f, 0.f };          // Current momentum (units/sec)
	sf::Vector2f mFacingDirection{ 1.f, 0.f };   // Normalized direction (default right)

	bool mIsGoalkeeper = false;       // True for players 3 (Home) and 7 (Away)
	bool mIsLunging = false;          // Dive / tackle animation flag
};

// ============================================================================
// SUMMARY OF SANTI CHANGES (Step 4)
// ============================================================================
// 1) Added init(int playerID, int teamID, bool isGoalkeeper):
//      - Sets IDs, team, goalkeeper flag.
//      - Initializes velocity, facing direction, lunging to safe defaults.
// 2) Added const getters required by World::writeRawState:
//      - getPlayerID(), getTeam(), getVelocity(), getFacingDirection(),
//        IsGoalkeeper(), IsLunging().
// 3) Added default initializers for all member fields to prevent uninitialized
//    snapshot data (mPlayerID = -1, mTeam = 0, mVelocity = zero,
//    mFacingDirection = (1,0), mIsGoalkeeper = false, mIsLunging = false).
// ============================================================================
