#pragma once

/**
 * @file World.h
 * @brief Owns the simulation sandbox: players, ball, goals, cows, and pitch bounds.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help draft/format documentation comments
 * and to propose a clean, readable public API boundary for World (object ownership + mechanics)
 * vs Match/MatchState (rules + transitions). The team implemented the simulation behavior and
 * validated it through play-testing.
 *
 * Example prompt used:
 * "Review this World class header for a C++/SFML soccer game. Suggest concise
 * Doxygen comments and a clear separation of responsibilities between World
 * (object ownership + mechanics) and Match/MatchState (rules + transitions)."
 */

#include "../Objects/Goal.h"
#include "../Objects/Ball.h"
#include "../Objects/Player.h"
#include "../Common/Packets.h"
#include <array>
#include <cstdint>

// ============================================================================
// WORLD
// ============================================================================
// The "sandbox" that owns all game objects (players, ball, goals, pitch bounds).
// Does NOT contain match rules (score, timer, state machine). That is Match's job.
//
// World can produce a GameStatePacket snapshot without owning match rules.
// ============================================================================

/**
 * @class World
 * @brief Simulation owner for physical game objects.
 *
 * World mutates objects and produces raw snapshots. Match owns score, timer,
 * current state, and higher-level rules.
 */
class World {
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION & RESET
	// ------------------------------------------------------------------------

	/** @brief Construct and initialize the pitch, goals, ball, players, and cows. */
	World();

	// Deterministic reset for kickoff.
	// kickoffTeamSide determines which team starts with the ball:
	// - Config::HOME_TEAM_SIDE -> player 0 starts on the center spot
	// - Config::AWAY_TEAM_SIDE -> player 4 starts on the center spot
	/**
	 * @brief Reset players and ball for a kickoff.
	 * @param kickoffTeamSide Team that starts with possession.
	 */
	void resetKickoff(int kickoffTeamSide = Config::HOME_TEAM_SIDE);

	// ------------------------------------------------------------------------
	// SNAPSHOT PRODUCTION (Step 4 deliverable)
	// ------------------------------------------------------------------------

	/**
	 * @brief Copy World-owned object state into a GameStatePacket.
	 * @param out Snapshot to fill.
	 */
	void writeRawState(GameStatePacket& out) const;

	// ------------------------------------------------------------------------
	// ACCESSORS (used by Step 5 Match / States)
	// ------------------------------------------------------------------------

	// Non-const access to the array of all players (home + away).
	std::array<Player, Config::kNumPlayers>& players() { return mPlayers; }

	// Const access for read-only operations.
	const std::array<Player, Config::kNumPlayers>& players() const { return mPlayers; }

	// Non-const access to the ball.
	Ball& ball() { return mBall; }

	// Const access to the ball.
	const Ball& ball() const { return mBall; }


	/** @brief Move all players based on a full-frame input snapshot. */
	void applyFrameMovement(const FrameInput& frameData, float dt);

	// ------------------------------------------------------------------------
	// POSSESSION / INTERACTION MECHANICS
	// ------------------------------------------------------------------------
	// These functions mutate World-owned objects (players + ball) but do NOT
	// change match rules (score, timer, state transitions). That separation
	// keeps the design OOP-first:
	// - World is the sandbox/arena and owns objects.
	// - MatchState decides WHEN to call these mechanics based on game rules.

	/** @brief If the ball has an owner, attach it to the owner's position. */
	void attachBallToOwnerIfAny();

	/** @brief If the ball is loose and a player is close enough, assign ownership. */
	void tryPickupLooseBall(float pickupRadius);

	/**
	 * @brief Resolve tackle/steal attempts against the current ball owner.
	 *
	 * Notes:
	 * - Only the host simulation decides steals.
	 * - Uses FrameInput.tackleDown as the trigger.
	 * - Team membership is derived from player ID ranges (0-3 home, 4-7 away).
	 *
	 * Call site: PlayingState::update, after movement + attachment, before pass/shot.
	 */
	void resolveTackleSteals(const FrameInput& frameData, float dt);

	/**
	 * @brief Restrict player positions while a goalkeeper is holding the ball.
	 *
	 * When the ball is owned by a goalkeeper, no other player (teammate or opponent)
	 * is allowed to enter that goalkeeper's six-yard box. This keeps distribution
	 * playable and avoids clustering behind the keeper.
	 */
	void enforceGoalkeeperSixYardBoxProtection();

	/**
	 * @brief Enforce that no players can stand inside the goal mouth (net area).
	 *
	 * This is enforced for both goals and for both teams each tick.
	 */
	void enforceNoPlayersInsideGoalMouth();

	/** @brief Mutable accessor for the left goal (home team defends). */
	Goal& homeGoal() { return mHomeGoal; }
	/** @brief Mutable accessor for the right goal (away team defends). */
	Goal& awayGoal() { return mAwayGoal; }
	/** @brief Const accessor for the left goal (home team defends). */
	const Goal& homeGoal() const { return mHomeGoal; }
	/** @brief Const accessor for the right goal (away team defends). */
	const Goal& awayGoal() const { return mAwayGoal; }

	/**
	 * @brief Start a guided pass using an interception corridor query.
	 *
	 * World computes an intended target and uses PhysicsEngine geometry to shorten
	 * the kick if a defender blocks the corridor.
	 */
	void kickGuaranteedPassWithInterception(int ownerId);

	/** @brief Start a guided shot using an interception corridor query. */
	void kickGuaranteedShotWithInterception(int ownerId);

	// ------------------------------------------------------------------------
	// KICKOFF RULES
	// ------------------------------------------------------------------------
	// The match is in KickoffState until the kicking team completes an opening pass.
	// During this phase, kickoff placement rules are enforced:
	// - Every non-kicker player stays in their own half (cannot cross midfield)
	// - Every non-kicker player stays outside the kickoff circle
	//
	/** @brief Enforce kickoff constraints on the defending team. */
	void enforceKickoffDefenderRestrictions(int kickoffTeamSide);

	/**
	 * @brief Start the kickoff opening pass (no interception query).
	 *
	 * Opponents cannot challenge inside the kickoff circle. The ball is assigned to
	 * the receiver only when the guided travel completes.
	 */
	void kickKickoffPassToTeammate(int kickoffOwnerId);

	/**
	 * @brief Host-side input edge helpers.
	 *
	 * Packets carry DOWN state; the host computes "pressed this frame" edges by
	 * comparing against the previous tick's DOWN state for the same player.
	 *
	 * Call commitActionButtonHistory(frameData) once per tick so edges are computed
	 * relative to the immediately previous frame.
	 */
	bool isPassPressed(const FrameInput& frameData, int playerId) const;
	bool isShootPressed(const FrameInput& frameData, int playerId) const;
	void commitActionButtonHistory(const FrameInput& frameData);

	// ------------------------------------------------------------------------
	// CHAOS EVENT: COWS
	// ------------------------------------------------------------------------
	// Host-only simulation of the "Copa Peru cows" chaos event.
	// Clients never simulate cows; they render CowState from the packet.
	//
	// Call site: PlayingState::update (host simulation only).
	void updateCows(float dt);

	// Cows block players (simple circle collision). This is enforced by pushing
	// players out of cow circles after movement/separation.
	void resolveCowPlayerCollisions(float dt);

	// Cows block the ball (including guided passes/shots).
	// Call after Ball::update(dt). Provide the previous ball position so we can
	// compute a reliable "effective velocity" even during guided travel.
	void resolveCowBallCollisions(float dt, const sf::Vector2f& prevBallPos);




private:
	// ------------------------------------------------------------------------
	// MEMBER VARIABLES (verbose names explain ownership)
	// ------------------------------------------------------------------------

	// Array of all 8 players (4 home, 4 away). Index 0-3 = Home, 4-7 = Away.
	std::array<Player, Config::kNumPlayers> mPlayers{};

	// The single ball on the pitch.
	Ball mBall{};

	// Left goal (Home team defends this).
	Goal mHomeGoal{};

	// Right goal (Away team defends this).
	Goal mAwayGoal{};

	// Playable field boundaries (same as Config::WINDOW_WIDTH/HEIGHT).
	// Stored here to be copied into GameStatePacket.
	sf::FloatRect mPitchBounds{};

	// ------------------------------------------------------------------------
	// COW RUNTIME STATE (host-authoritative)
	// ------------------------------------------------------------------------
	// We keep cow simulation state inside World so the host remains authoritative.
	// Networking stays simple by exporting a fixed-size array of CowState snapshots.
	struct CowRuntime {
		bool active = false;
		sf::Vector2f position{ 0.f, 0.f };
		sf::Vector2f velocity{ 0.f, 0.f };
		sf::Vector2f target{ 0.f, 0.f };
		// Phase timer is reused for:
		// - how long the cow keeps moving before pausing
		// - how long the cow pauses before picking a new target
		float phaseTimerSec = 0.f;

		// 0 = entering (from outside pitch to first target)
		// 1 = moving (wandering to a random target)
		// 2 = paused (standing still, then pick a new target)
		std::uint8_t phase = 0;
		std::uint8_t entrySide = 0; // 0 left, 1 right, 2 top, 3 bottom
	};

	std::array<CowRuntime, Config::kMaxCows> mCows{};

	// Countdown to spawn the NEXT cow. Once kMaxCows are active, no more spawns happen.
	float mCowNextSpawnCountdownSec = 0.f;

	// Simple per-player retry cooldown to avoid constant steal spam.
	// Indexed by player ID (0-7). Counts down in seconds.
	std::array<float, Config::kNumPlayers> mStealRetryCooldownSec{};

	// Per-player DOWN-state history used to compute edges.
	std::array<bool, Config::kNumPlayers> mWasPassDown{};
	std::array<bool, Config::kNumPlayers> mWasShootDown{};
};
