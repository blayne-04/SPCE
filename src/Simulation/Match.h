#pragma once

#include "World.h"
#include "States/MatchState.h"
#include "../Common/Packets.h"
#include "../Common/Types.h"

class Match {

public:

	Match() : homeScore(0), awayScore(0), matchTimer(0.0f), mCurrentState(nullptr)
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
	void clearScore() { homeScore = 0; awayScore = 0; }

	/* Timer helpers */
	void incrementTimer() { matchTimer++; }
	void clearTimer() { matchTimer = 0.0f; }
	float getTimer() const { return matchTimer; }

	/* Returns reference to world, for state class to mutate and interface with */
	World& getWorld() { return mWorld; }

private:

	World mWorld;

	int mHomeScore;
	int mAwayScore;

<<<<<<< HEAD
	float mMatchTimer;
	bool mIsOverTimer;
=======
	float matchTimer;
>>>>>>> 568018f6c5728e5b4394680b5e9fea8a92c240eb

	std::unique_ptr<MatchState> mCurrentState;
};
