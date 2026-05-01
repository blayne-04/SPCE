#pragma once

/**
 * @file Match.h
 * @brief Referee/controller for World, MatchState, score, timer, and control routing.
 *
 * AI assistance disclosure:
 * A generative AI assistant (DeepSeek) was used in a limited way to help draft/format documentation
 * comments and to sanity-check state-machine wiring patterns (Kickoff/Playing/GameOver).
 * The team designed the match architecture and implemented the gameplay rules.
 *
 * Prompt used:
 * "Review this Match class header. Suggest concise Doxygen comments and highlight common pitfalls
 * when wiring a state machine that owns World + score + timer. Do not change runtime behavior."
 */

#include "World.h"
#include "../States/MatchStates.h"
#include "../Common/Packets.h"
#include "../Common/Types.h"
#include <memory>
#include <cstdint>

 /**
  * @class Match
  * @brief Owns match rules and delegates state-specific behavior to MatchState.
  */
class Match {

public:

	/** @brief Construct a match and enter the initial state. */
	Match();

	~Match() = default;

	/**
	 * @brief Reset all match-owned gameplay data and return to kickoff.
	 *
	 * This is used when leaving a match through pause/game-over menus. GameEngine
	 * owns one Match object for the program lifetime, so selecting a new play
	 * mode must explicitly clear the old score, timer, World, and MatchState.
	 */
	void reset();

	/* Pass data from gameEngine all the way down the pipeline, delegate to state class */
	/**
	 * @brief Advance the match by one simulation tick.
	 * @param frameData Inputs for all players.
	 * @param dt Delta time in seconds.
	 */
	void update(const FrameInput& frameData, float dt);

	/* Handle state transitions, called by state classes to transition to next state */
	/**
	 * @brief Change the current MatchState.
	 * @param nextState New state object.
	 */
	void TransitionTo(std::unique_ptr<MatchState> nextState);

	/* Adjust score for specified team */
	/** @brief Add one goal to the requested team. */
	void incrementScore(TEAMS side);
	void clearScore() { mHomeScore = 0; mAwayScore = 0; }

	/**
	 * @brief Fill an authoritative GameStatePacket snapshot.
	 * @param out Snapshot to write.
	 */
	void getGameState(GameStatePacket& out) const;

	/* Timer helpers */
	void addTimeSec(float dt) { mMatchTimerSec += dt; }
	void clearTimerSec() { mMatchTimerSec = 0.f; }
	float getTimerSec() const { return mMatchTimerSec; }

	/*void overwriteWorld(GameStatePacket incomingHostPacket) { mWorld.overwriteFromPacket(incomingHostPacket);}*/ // REMOVED FOR NOW

	/* Returns reference to world, for state class to mutate and interface with */
	World& getWorld() { return mWorld; }

	const World& getWorld() const { return mWorld; }

	void setControlledPlayerIds(std::uint8_t homeId, std::uint8_t awayId);

	/** @brief Current input-routing IDs (used by EngineState to map local input). */
	std::uint8_t getControlledHomePlayerId() const { return mControlledHomePlayerId; }
	std::uint8_t getControlledAwayPlayerId() const { return mControlledAwayPlayerId; }

	/** @brief Kickoff side controls who starts with the ball on restart (0 = Home, 1 = Away). */
	void setKickoffTeamSide(std::uint8_t teamSide) { mKickoffTeamSide = teamSide; }
	std::uint8_t getKickoffTeamSide() const { return mKickoffTeamSide; }


private:

	World mWorld;

	std::uint16_t mHomeScore = 0;
	std::uint16_t mAwayScore = 0;

	float mMatchTimerSec = 0.f;
	std::uint32_t mFrameNumber = 0;

	bool mIsOverTime = false;

	std::int8_t mPossessingTeamId = -1;
	std::uint8_t mControlledHomePlayerId = 0;
	std::uint8_t mControlledAwayPlayerId = 4;

	// Host-authoritative edge detection for switchDown (I key).
	// InputPacket is "button down"; Match computes "pressed this frame" from this history.
	bool mWasHomeSwitchDown = false;
	bool mWasAwaySwitchDown = false;

	// Manual defensive toggle state.
	// false = control nearest outfield defender, true = control second-nearest outfield defender.
	bool mHomeDefenseSecondClosest = false;
	bool mAwayDefenseSecondClosest = false;

	// Which team takes the next kickoff (0 = Home, 1 = Away).
	// Real football: the conceding team kicks off after a goal.
	std::uint8_t mKickoffTeamSide = Config::HOME_TEAM_SIDE;

	std::unique_ptr<MatchState> mCurrentState;
};
