#pragma once

#include "../Objects/Goal.h"
#include "../Objects/Ball.h"
#include "../Objects/Player.h"
#include "../Common/Packets.h"
#include <array>

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
	void resetKickoff();

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
    void overwriteWorldFromPacket(GameStatePacket incomingHostPacket);
    // Your methods here...

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
