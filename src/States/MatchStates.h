#pragma once
/**
 * @file MatchStates.h
 * @brief Polymorphic match-state classes for kickoff, playing, and game over.
 *
 * AI assistance disclosure:
 * A generative AI assistant (Gemini) was used in a limited way to help draft/format documentation
 * comments and to sanity-check the base-class shape for a simple match rule state machine
 * (Kickoff/Playing/GameOver). The team defined the architecture and packet contract, then
 * implemented and tested the behavior.
 *
 * Prompt used:
 * "Review this MatchState header for a C++/SFML soccer game. Suggest concise Doxygen comments
 * and a clean base-class API (update/onEnter/onExit + packet state id) without changing any
 * runtime behavior."
 */

#include "../Common/Packets.h"
#include <cstdint> // for std::uint8_t

 /* Forward declaration to avoid circular dependency */
class Match;

/* ========================================= */
/*              BASE CLASS                   */
/* ========================================= */
/**
 * @class MatchState
 * @brief Abstract base class for match-rule states.
 */
class MatchState {
public:
	MatchState() = default;

	virtual ~MatchState() = default;

	/** @brief Numeric state ID copied into GameStatePacket.currentState. */
	virtual std::uint8_t packetStateId() const = 0;

	/**
	 * @brief Apply one tick of state-specific match logic.
	 * @param match Match/referee object that owns World and score/timer state.
	 * @param frameData Input for every player this tick.
	 * @param dt Delta time in seconds.
	 */
	virtual void update(Match& match, const FrameInput& frameData, float dt) = 0;

	/** @brief Hook called immediately after transitioning into this state. */
	virtual void onEnter(Match& match) = 0;

	/** @brief Hook called immediately before transitioning out of this state. */
	virtual void onExit(Match& match) = 0;
};

/* ========================================= */
/*              GAME OVER                    */
/* ========================================= */

/**
 * @class GameOverState
 * @brief Frozen terminal state after match time expires.
 */
class GameOverState : public MatchState
{
public:
	GameOverState();
	std::uint8_t packetStateId() const override { return 3; }
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;
};

/* ========================================= */
/*                KICKOFF                    */
/* ========================================= */
/**
 * @class KickoffState
 * @brief Restart state that enforces kickoff placement and opening pass rules.
 */
class KickoffState : public MatchState
{
public:
	KickoffState();
	std::uint8_t packetStateId() const override { return 0; }
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;

private:
	// Kickoff is not complete until the opening pass begins (ball is played to a teammate).
	bool mKickoffPassStarted = false;
};

/* ========================================= */
/*                PLAYING                    */
/* ========================================= */
/**
 * @class PlayingState
 * @brief Main simulation state for movement, possession, cows, goals, and timer.
 */
class PlayingState : public MatchState
{
public:
	PlayingState();
	std::uint8_t packetStateId() const override { return 1; }
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;
};
