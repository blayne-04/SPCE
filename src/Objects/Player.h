#pragma once

/**
 * @file Player.h
 * @brief Player entity state and movement behavior.
 */

#include "SFML/Graphics.hpp"
#include "../Common/Constants.h" // Config tuning table
#include <algorithm>             // std::clamp

// ============================================================================
// PLAYER CLASS
// ============================================================================
// Represents a single player on the field (outfield or goalkeeper).
// Inherits from sf::CircleShape for rendering.
// Owns physics state (position inherited from sf::CircleShape, plus velocity,
// facing direction, lunging flag, team ID, player ID, goalkeeper flag).
//
// This class supports snapshotting by exposing const getters for key state.
// ============================================================================

/**
 * @class Player
 * @brief Represents one home or away player, including goalkeepers.
 *
 * Player owns per-player physical state. World decides when movement is applied.
 */
class Player : public sf::CircleShape {
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION
	// ------------------------------------------------------------------------
	/** @brief Default constructor; call init() before gameplay use. */
	Player() = default;

	/**
	 * @brief Initialize stable IDs, team, goalkeeper flag, and visual radius.
	 */
	void init(int playerID, int teamID, bool isGoalkeeper) {
		mPlayerID = playerID;
		mTeam = teamID;
		mIsGoalkeeper = isGoalkeeper;

		mIsLunging = false;
		mVelocity = sf::Vector2f(0.f, 0.f);
		mFacingDirection = sf::Vector2f(1.f, 0.f);

		// Positions represent the CENTER of the circle.
		setRadius(Config::PLAYER_HALF_SIZE);
		setOrigin(sf::Vector2f(getRadius(), getRadius()));
	}

	// ------------------------------------------------------------------------
	// PUBLIC INTERFACE (not yet implemented fully)
	// ------------------------------------------------------------------------
	/** @brief Legacy placeholder update hook. Movement currently comes from World. */
	void update(float downTime);

	/** @brief Legacy placeholder kick hook. Ball actions currently live in World/Ball. */
	void kickBall();

	// ------------------------------------------------------------------------
	// CONST GETTERS (used by World::writeRawState snapshot)
	// ------------------------------------------------------------------------
	int getPlayerID() const { return mPlayerID; }
	int getTeam() const { return mTeam; }

	sf::Vector2f getVelocity() const { return mVelocity; }
	sf::Vector2f getFacingDirection() const { return mFacingDirection; }

	bool IsGoalkeeper() const { return mIsGoalkeeper; }
	bool IsLunging() const { return mIsLunging; }

	/**
	 * @brief Apply one tick of movement from normalized input direction.
	 * @param moveDir Normalized desired movement direction.
	 * @param dt Delta time in seconds.
	 */
	void applyMoveDirection(const sf::Vector2f& moveDir, float dt) {
		if (dt <= 0.f) return;

		// Snapshot-friendly velocity (even if you don't use it for physics yet).
		mVelocity = moveDir * Config::PLAYER_SPEED;

		const bool isMoving = (moveDir.x != 0.f || moveDir.y != 0.f);
		if (isMoving) {
			// moveDir is already normalized by InputHandler.
			mFacingDirection = moveDir;
		}

		const sf::Vector2f originalPos = getPosition();
		sf::Vector2f pos = originalPos + (mVelocity * dt);

		// Goalkeepers should not roam outside the goal mouth: keeper X stays fixed,
		// and Y is clamped within the goal opening (between posts).
		if (mIsGoalkeeper) {
			// X must be fixed to the keeper's home/away slot (not the current position),
			// so separation pushes cannot permanently drift the goalkeeper away from the goal.
			const bool isHomeKeeper = (mTeam == Config::HOME_TEAM_SIDE);
			pos.x = isHomeKeeper ? Config::GOALKEEPER_X_LEFT : Config::GOALKEEPER_X_RIGHT;
			mVelocity.x = 0.f;

			const float yMin = Config::GOAL_Y_TOP + Config::PLAYER_HALF_SIZE;
			const float yMax = Config::GOAL_Y_BOTTOM - Config::PLAYER_HALF_SIZE;
			pos.y = std::clamp(pos.y, yMin, yMax);
		}
		else {
			pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
			pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
		}
		setPosition(pos);
	}


private:
	// ------------------------------------------------------------------------
	// MEMBER VARIABLES (verbose names, all default-initialized)
	// ------------------------------------------------------------------------

	// Defaults prevent uninitialized snapshot data.
	int mPlayerID = -1;               // 0-7 (0-3 Home, 4-7 Away)
	int mTeam = 0;                    // 0 = Home, 1 = Away

	sf::Vector2f mVelocity{ 0.f, 0.f };          // Current momentum (units/sec)
	sf::Vector2f mFacingDirection{ 1.f, 0.f };   // Normalized direction (default right)

	bool mIsGoalkeeper = false;       // True for players 3 (Home) and 7 (Away)
	bool mIsLunging = false;          // Dive / tackle animation flag
};
