#include "Match.h"
#include <utility> // std::move
#include <type_traits>

Match::Match() {
	// Start in Kickoff by default.
	mWorld = World();
	TransitionTo(std::make_unique<KickoffState>());
}

void Match::setControlledPlayerIds(std::uint8_t homeId, std::uint8_t awayId) {
	mControlledHomePlayerId = homeId;
	mControlledAwayPlayerId = awayId;
}

void Match::TransitionTo(std::unique_ptr<MatchState> nextState) {
	if (!nextState) return; // SANTI: guard

	if (mCurrentState) {
		mCurrentState->onExit(*this);
	}

	mCurrentState = std::move(nextState);
	mCurrentState->onEnter(*this);
}

void Match::incrementScore(TEAMS side) {
	if (side == TEAMS::TEAM1) {
		++mHomeScore;
		return;
	}
	++mAwayScore;
}

void Match::update(const FrameInput& frameData, float dt) {
	// SANTI: keep frame number monotonic for networking.
	++mFrameNumber;

	if (!mCurrentState) {
		TransitionTo(std::make_unique<KickoffState>());
		return;
	}

	mCurrentState->update(*this, frameData, dt);

	// SANTI: update possessingTeamId from ball owner when available.
	const int owner = mWorld.ball().getOwner();
	if (owner < 0) return;

	// 0-3 home, 4-7 away.
	mPossessingTeamId = (owner < 4) ? 0 : 1;
}

void Match::getGameState(GameStatePacket& out) const {
	// Fill world-owned fields first.
	mWorld.writeRawState(out);

	// Then fill match metadata.
	out.frameNumber = mFrameNumber;
	out.scoreHome = mHomeScore;
	out.scoreAway = mAwayScore;
	out.matchTimerSec = mMatchTimerSec;

	out.possessingTeamId = mPossessingTeamId;
	out.controlledHomePlayerId = mControlledHomePlayerId;
	out.controlledAwayPlayerId = mControlledAwayPlayerId;

	out.currentState = mCurrentState ? mCurrentState->packetStateId() : 0;
}
