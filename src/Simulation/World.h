#pragma once

#include "../Objects/Goal.h"
#include "../Objects/Ball.h"
#include "../Objects/Player.h"
#include "../Common/Packets.h"
#include <array>
#include <cstdint> // SANTI: COWS 29/04/26

// ============================================================================
// WORLD
// ============================================================================
// The "sandbox" that owns all game objects (players, ball, goals, pitch bounds).
// Does NOT contain match rules (score, timer, state machine). That is Match's job.
//
// SANTI: Step 4 deliverable - World now holds the raw state and can produce
//        a GameStatePacket snapshot without any match logic.
// ============================================================================

class World {
public:
	// ------------------------------------------------------------------------
	// CONSTRUCTION & RESET
	// ------------------------------------------------------------------------

	// SANTI: Declare explicitly because we define it in World.cpp (MSVC requires this).
	World();

	// SANTI: Deterministic reset for kickoff.
	//        Resets all objects to their initial positions (home/away formations).
	// SANTI 28/04/2026: kickoffTeamSide determines which team starts with the ball:
	// - Config::HOME_TEAM_SIDE -> player 0 starts on the center spot
	// - Config::AWAY_TEAM_SIDE -> player 4 starts on the center spot
	void resetKickoff(int kickoffTeamSide = Config::HOME_TEAM_SIDE);

	// ------------------------------------------------------------------------
	// SNAPSHOT PRODUCTION (Step 4 deliverable)
	// ------------------------------------------------------------------------

	// SANTI: Fill snapshot fields derived from World objects.
	//        Copies current player states, ball state, and pitch bounds into 'out'.
	//        Called by Match::getGameState() each tick.
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


	// SANTI: Step 5 helper. Moves all players based on FrameInput.
	void applyFrameMovement(const FrameInput& frameData, float dt);

	// ------------------------------------------------------------------------
	// POSSESSION MECHANICS (SANTI: Step 6 gameplay wiring)
	// ------------------------------------------------------------------------
	// These functions mutate World-owned objects (players + ball) but do NOT
	// change match rules (score, timer, state transitions). That separation
	// keeps the design OOP-first:
	// - World is the sandbox/arena and owns objects.
	// - MatchState decides WHEN to call these mechanics based on game rules.

	// SANTI: If the ball has an owner, attach it to the owner's position.
	// This makes "possession" a consistent physical representation.
	void attachBallToOwnerIfAny();

	// SANTI: If the ball is loose and a player is close enough, assign ownership.
	// This is the MVP version of "ball pickup / interception".
	void tryPickupLooseBall(float pickupRadius);

	// SANTI 28/04/2026: Resolve tackle/steal attempts against the current ball owner.
	// This is the simplified port of the old project's PossessionResolver/StealResolver:
	// - Only the host simulation decides steals.
	// - Uses FrameInput.tackleDown as the trigger.
	// - No Team class: team membership is derived from player ID ranges.
	//
	// Call site: PlayingState::update, after movement + attachment, before pass/shot.
	void resolveTackleSteals(const FrameInput& frameData, float dt);

	// SANTI 28/04/2026: Goalkeeper holding protection.
	// When the ball is owned by a goalkeeper, NO other player (teammate or opponent)
	// is allowed to enter that goalkeeper's six-yard box. This keeps distribution
	// playable and avoids "tackle spam" and weird clustering behind the keeper.
	//
	// Call site: PlayingState::update, after movement and after ball attachment.
	void enforceGoalkeeperSixYardBoxProtection();

	// SANTI 28/04/2026: Hard rule: no players should ever be able to stand "inside"
	// the goal mouth (inside the net area). This was a visible issue in playtests:
	// teammates (and sometimes opponents) could drift into the goal rectangle,
	// which does not make football sense and breaks immersion.
	//
	// This is enforced for BOTH goals, for BOTH teams, every tick.
	// Call site: MatchStates (KickoffState + PlayingState), after movement/separation.
	void enforceNoPlayersInsideGoalMouth();

	Goal& homeGoal() { return mHomeGoal; }          // SANTI
	Goal& awayGoal() { return mAwayGoal; }          // SANTI
	const Goal& homeGoal() const { return mHomeGoal; } // SANTI
	const Goal& awayGoal() const { return mAwayGoal; } // SANTI

	// SANTI: "Guaranteed pass" helper used by PlayingState when the owner presses passDown.
	// World computes a simple intended target and then applies a PhysicsEngine interception
	// query to potentially shorten the kick if a defender blocks the corridor.
	void kickGuaranteedPassWithInterception(int ownerId);

	// SANTI: "Guaranteed shot" helper used by PlayingState when the owner presses shootDown.
	// World aims at opponent goal center and applies the same interception query idea.
	void kickGuaranteedShotWithInterception(int ownerId);

	// ------------------------------------------------------------------------
	// KICKOFF RULES (SANTI 28/04/2026)
	// ------------------------------------------------------------------------
	// The match is in KickoffState until the kicking team completes an opening pass.
	// During this phase, kickoff placement rules are enforced:
	// - Every non-kicker player stays in their own half (cannot cross midfield)
	// - Every non-kicker player stays outside the kickoff circle
	//
	// KickoffState calls these helpers to keep MatchStates.cpp readable.
	void enforceKickoffDefenderRestrictions(int kickoffTeamSide);

	// SANTI 28/04/2026: Starts the kickoff opening pass (no interception query).
	// This mirrors real football: opponents cannot challenge inside the kickoff circle.
	// The ball will be assigned to the receiver only when the guided travel completes.
	void kickKickoffPassToTeammate(int kickoffOwnerId);

	// SANTI 28/04/2026: Host-side input edge helpers.
	// Packets carry DOWN state; host computes "pressed this frame" by comparing
	// against the previous tick's DOWN state for that same player.
	//
	// IMPORTANT: Call commitActionButtonHistory(frameData) once per tick (PlayingState)
	// so edges are computed relative to the immediately previous frame.
	bool isPassPressed(const FrameInput& frameData, int playerId) const;
	bool isShootPressed(const FrameInput& frameData, int playerId) const;
	void commitActionButtonHistory(const FrameInput& frameData);

	// ------------------------------------------------------------------------
	// CHAOS EVENT: COWS (SANTI: COWS 29/04/26)
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

	// SANTI: Array of all 8 players (4 home, 4 away). Index 0-3 = Home, 4-7 = Away.
	std::array<Player, Config::kNumPlayers> mPlayers{};

	// SANTI: The single ball on the pitch.
	Ball mBall{};

	// SANTI: Left goal (Home team defends this).
	Goal mHomeGoal{};

	// SANTI: Right goal (Away team defends this).
	Goal mAwayGoal{};

	// SANTI: Playable field boundaries (same as Config::WINDOW_WIDTH/HEIGHT).
	//        Stored here to be copied into GameStatePacket.
	sf::FloatRect mPitchBounds{};

	// ------------------------------------------------------------------------
	// COWS (SANTI: COWS 29/04/26)
	// ------------------------------------------------------------------------
	// We keep cow simulation state inside World so the host remains authoritative.
	// Networking stays simple by exporting a fixed-size array of CowState snapshots.
	struct CowRuntime {
		bool active = false;
		sf::Vector2f position{ 0.f, 0.f };
		sf::Vector2f velocity{ 0.f, 0.f };
		sf::Vector2f target{ 0.f, 0.f };
		// SANTI: COWS 30/04/26
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

	// SANTI: COWS 30/04/26
	// Countdown to spawn the NEXT cow. Once kMaxCows are active, no more spawns happen.
	float mCowNextSpawnCountdownSec = 0.f;

	// SANTI 28/04/2026: Simple per-player retry cooldown to avoid constant steal spam.
	// Indexed by player ID (0-7). Counts down in seconds.
	std::array<float, Config::kNumPlayers> mStealRetryCooldownSec{};

	// SANTI 28/04/2026: Per-player DOWN-state history used to compute edges.
	std::array<bool, Config::kNumPlayers> mWasPassDown{};
	std::array<bool, Config::kNumPlayers> mWasShootDown{};
};

// ============================================================================
// SUMMARY OF SANTI CHANGES (Step 4)
// ============================================================================
// 1. Turned World from empty stub into actual object owner.
// 2. Added ownership members:
//      - mPlayers (array of 8 players)
//      - mBall
//      - mHomeGoal, mAwayGoal
//      - mPitchBounds
// 3. Added Step 4 API:
//      - World() constructor (defined in World.cpp)
//      - resetKickoff() - deterministic initialization
//      - writeRawState() - produces snapshot without match logic
// 4. Added simple accessors players() and ball() for Step 5/Match/States.
// ============================================================================
