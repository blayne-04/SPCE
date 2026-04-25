#pragma once

#include "World.h"
#include "States/MatchState.h"

#include <memory>

class Match {

public:

	Match() = default;

	void update(const FrameInput& frameData);

	void applyNetworkState(const GameStatePacket& packet);

	GameStatePacket exportNetworkState(unique32_t seqNum);

	void transitionTo(int stateID);

	void incrementScore(int side);

	setNextKickingSide(int side);

	World& getWorld() { return mWorld; }

	int getCurrentStateID();




private:

	World mWorld;

	int homeScore;
	int awayScore;

	float matchTimer;
	bool isOverTimer;

	unique_ptr<MatchState> currentState;

	int nextKickingSide;
};
