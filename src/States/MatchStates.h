#pragma once
#include "../Common/Packets.h"
#include <cstdint> // SANTI: for std::uint8_t

/* Forward declaration to avoid circular dependency */
class Match;

/* ========================================= */
/*              BASE CLASS                   */
/* ========================================= */
class MatchState {
public:
	MatchState() = default;

	virtual ~MatchState() = default;

	// SANTI: Needed so Match can fill GameStatePacket.currentState.
	virtual std::uint8_t packetStateId() const = 0;

	/* Contextual update function, called by Match to delegate frame updates to the current state of the program */
	// SANTI: dt is required for matchTimerSec counting up.
	virtual void update(Match& match, const FrameInput& frameData, float dt) = 0;

	/* Contextual function to be called when entering a new state */
	virtual void onEnter(Match& match) = 0;

	/* Contextual function to be called when exiting a state */
	virtual void onExit(Match& match) = 0;
};

/* ========================================= */
/*              GAME OVER                    */
/* ========================================= */

class GameOverState : public MatchState
{
public:
	GameOverState();
	std::uint8_t packetStateId() const override { return 3; } // SANTI
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;
};

/* ========================================= */
/*                KICKOFF                    */
/* ========================================= */
class KickoffState : public MatchState
{
public:
	KickoffState();
	std::uint8_t packetStateId() const override { return 0; } // SANTI
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;

private:
	// SANTI 28/04/2026: Kickoff is not complete until the opening pass finishes.
	// This mirrors real football: play begins after the ball is played to a teammate.
	bool mKickoffPassStarted = false;
};

/* ========================================= */
/*                PLAYING                    */
/* ========================================= */
class PlayingState : public MatchState
{
public:
	PlayingState();
	std::uint8_t packetStateId() const override { return 1; } // SANTI
	void update(Match& match, const FrameInput& frameData, float dt) override;
	void onEnter(Match& match) override;
	void onExit(Match& match) override;
};
