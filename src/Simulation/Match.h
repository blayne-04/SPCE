#pragma once

#include "World.h"
#include "../States/MatchStates.h"
#include "../Common/Packets.h"
#include "../Common/Types.h"
#include <memory>
#include <cstdint> // SANTI

class Match {

public:

	Match();

	//Match() :mHomeScore(0), mAwayScore(0), mMatchTimerSec(0.0f), mIsOverTime(false), mCurrentState(nullptr)
	//{
	//	//mWorld = World();
	//}

	~Match() = default;

	/* Pass data from gameEngine all the way down the pipeline, delegate to state class */
	// SANTI: dt is required for matchTimerSec.
	void update(const FrameInput& frameData, float dt);

	/* Handle state transitions, called by state classes to transition to next state */
	void TransitionTo(std::unique_ptr<MatchState> nextState);

	/* Adjust score for specified team */
	void incrementScore(TEAMS side);
	void clearScore() { mHomeScore = 0; mAwayScore = 0; }

	// SANTI: output-by-reference snapshot (your preference).
	void getGameState(GameStatePacket& out) const;

	/* Timer helpers */
	void addTimeSec(float dt) { mMatchTimerSec += dt; }
	void clearTimerSec() { mMatchTimerSec = 0.f; }
	float getTimerSec() const { return mMatchTimerSec; }

	/*void overwriteWorld(GameStatePacket incomingHostPacket) { mWorld.overwriteFromPacket(incomingHostPacket);}*/ // REMOVED FOR NOW

	/* Returns reference to world, for state class to mutate and interface with */
	World& getWorld() { return mWorld; }

	const World& getWorld() const { return mWorld; } // SANTI: handy

	// SANTI: host can set these, and snapshot will carry them.
	void setControlledPlayerIds(std::uint8_t homeId, std::uint8_t awayId);

	// SANTI 28/04/2026: Read current control routing (used by EngineState to map local input).
	std::uint8_t getControlledHomePlayerId() const { return mControlledHomePlayerId; }
	std::uint8_t getControlledAwayPlayerId() const { return mControlledAwayPlayerId; }

	// SANTI 28/04/2026: Kickoff side controls who starts with the ball on restart.
	// 0 = Home, 1 = Away.
	void setKickoffTeamSide(std::uint8_t teamSide) { mKickoffTeamSide = teamSide; }
	std::uint8_t getKickoffTeamSide() const { return mKickoffTeamSide; }


private:

	World mWorld;

	std::uint16_t mHomeScore = 0; // SANTI: matches packet types
	std::uint16_t mAwayScore = 0;

	float mMatchTimerSec = 0.f;   // SANTI: counts up in seconds
	std::uint32_t mFrameNumber = 0;

	bool mIsOverTime = false; // SANTI: future - extra time / overtime rules

	std::int8_t mPossessingTeamId = -1; // SANTI: stable across loose ball if desired
	std::uint8_t mControlledHomePlayerId = 0; // defaults align with your handshake
	std::uint8_t mControlledAwayPlayerId = 4;

	// SANTI 28/04/2026: Host-authoritative edge detection for switchDown (I key).
	// InputPacket is "button down"; Match computes "pressed this frame" from this history.
	bool mWasHomeSwitchDown = false;
	bool mWasAwaySwitchDown = false;

	// SANTI 28/04/2026: Manual defensive toggle state.
	// false = control nearest outfield defender, true = control second-nearest outfield defender.
	bool mHomeDefenseSecondClosest = false;
	bool mAwayDefenseSecondClosest = false;

	// SANTI 28/04/2026: Which team takes the next kickoff (0 = Home, 1 = Away).
	// Real football: the conceding team kicks off after a goal.
	std::uint8_t mKickoffTeamSide = Config::HOME_TEAM_SIDE;

	std::unique_ptr<MatchState> mCurrentState;
};
