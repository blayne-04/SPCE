#pragma once

#include "World.h"
#include "../States/MatchStates.h"
#include "../Common/Packets.h"
#include "../Common/Types.h"


class Match {

public:

	Match() :mHomeScore(0), mAwayScore(0), mMatchTimer(0.0f), mIsOverTime(false), mCurrentState(nullptr)
	{
		mWorld = World();
	}

	~Match() = default;

	/* Pass data from gameEngine all the way down the pipeline, delegate to state class */
	void update(const FrameInput& frameData);

	/* Handle state transitions, called by state classes to transition to next state */
	void TransitionTo(std::unique_ptr<MatchState> nextState);

	/* Adjust score for specified team */
	void incrementScore(TEAMS side);
	void clearScore() { mHomeScore = 0; mAwayScore = 0; }

	/* Timer helpers */
	void incrementTimer() { mMatchTimer++; }
	void clearTimer() { mMatchTimer = 0.0f; }
	float getTimer() const { return mMatchTimer; }

	/* Returns reference to world, for state class to mutate and interface with */
	World& getWorld() { return mWorld; }

private:

	World mWorld;

	int mHomeScore;
	int mAwayScore;

	float mMatchTimer;
	bool mIsOverTime;

	std::unique_ptr<MatchState> mCurrentState;
};
