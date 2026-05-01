#include "Match.h"

/**
 * @file Match.cpp
 * @brief Match referee implementation.
 *
 * AI disclosure:
 * The state-delegation update flow, control switching, and snapshot fill logic
 * were generated/revised with help from OpenAI Codex.
 *
 * Prompt used:
 * "Help me implement a Match referee for a host-authoritative soccer game.
 * Match should own World, current MatchState, score, timer, control routing,
 * and fill GameStatePacket without putting match rules in GameEngine."
 */

#include <utility> // std::move
#include <type_traits>
#include <limits>  // SANTI 28/04/2026: std::numeric_limits

namespace {
	// SANTI 28/04/2026: 0-3 = Home team, 4-7 = Away team.
	static bool isHomeId(int id) {
		return id >= 0 && id < 4;
	}

	static float distSq(sf::Vector2f a, sf::Vector2f b) {
		const sf::Vector2f d = a - b;
		return d.x * d.x + d.y * d.y;
	}

	// SANTI 28/04/2026: Finds the nearest and second-nearest OUTFIELD players on the team.
	// Goalkeepers are excluded because you only want GK control when they have possession.
	static void findNearestOutfieldDefenders(
		const World& world,
		bool teamIsHome,
		const sf::Vector2f& ballPos,
		int& outNearestId,
		int& outSecondNearestId)
	{
		outNearestId = -1;
		outSecondNearestId = -1;

		const int start = teamIsHome ? 0 : 4;
		const int endInclusive = teamIsHome ? 3 : 7;
		const int goalkeeperId = teamIsHome ? 3 : 7;

		int bestId = -1;
		int secondId = -1;
		float bestD2 = std::numeric_limits<float>::max();
		float secondD2 = std::numeric_limits<float>::max();

		const auto& ps = world.players();
		for (int id = start; id <= endInclusive; ++id) {
			if (id == goalkeeperId) continue;

			const float d2 = distSq(ps[static_cast<std::size_t>(id)].getPosition(), ballPos);
			if (d2 < bestD2) {
				secondD2 = bestD2;
				secondId = bestId;

				bestD2 = d2;
				bestId = id;
				continue;
			}

			if (d2 < secondD2) {
				secondD2 = d2;
				secondId = id;
			}
		}

		outNearestId = bestId;
		outSecondNearestId = secondId;
	}
}

Match::Match() {
	// Start in Kickoff by default.
	reset();
}

void Match::reset() {
	// SANTI 01/05/2026
	// GameEngine keeps the same Match object while menus/states change, so a
	// new play mode must explicitly clear every piece of match-owned state.
	// This prevents old scores, timers, cows, possession, and MatchState objects
	// from leaking into the next match after exiting to the main menu.
	mWorld = World();
	mHomeScore = 0;
	mAwayScore = 0;
	mMatchTimerSec = 0.f;
	mFrameNumber = 0;
	mIsOverTime = false;
	mPossessingTeamId = -1;
	mControlledHomePlayerId = 0;
	mControlledAwayPlayerId = 4;
	mWasHomeSwitchDown = false;
	mWasAwaySwitchDown = false;
	mHomeDefenseSecondClosest = false;
	mAwayDefenseSecondClosest = false;
	mKickoffTeamSide = Config::HOME_TEAM_SIDE;
	mCurrentState.reset();
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
	}

	// SANTI 28/04/2026: Save the possession team before sim step so we can detect turnovers.
	const std::int8_t prevPossessingTeamId = mPossessingTeamId;

	// SANTI 28/04/2026: Compute "switch pressed this frame" for each team using DOWN-state input.
	// NOTE: Switch affects the NEXT tick's input routing (InputPacket is per-player).
	const std::uint8_t homeControlledThisTick = mControlledHomePlayerId;
	const std::uint8_t awayControlledThisTick = mControlledAwayPlayerId;

	bool homeSwitchDown = false;
	bool awaySwitchDown = false;

	if (homeControlledThisTick < Config::kNumPlayers) {
		homeSwitchDown = frameData.inputs[homeControlledThisTick].switchDown;
	}
	if (awayControlledThisTick < Config::kNumPlayers) {
		awaySwitchDown = frameData.inputs[awayControlledThisTick].switchDown;
	}

	const bool homeSwitchPressed = homeSwitchDown && !mWasHomeSwitchDown;
	const bool awaySwitchPressed = awaySwitchDown && !mWasAwaySwitchDown;

	mWasHomeSwitchDown = homeSwitchDown;
	mWasAwaySwitchDown = awaySwitchDown;

	mCurrentState->update(*this, frameData, dt);

	// SANTI: update possessingTeamId from ball owner when available.
	const int owner = mWorld.ball().getOwner();
	if (owner >= 0) {
		// 0-3 home, 4-7 away.
		mPossessingTeamId = (owner < 4) ? 0 : 1;
	}

	// SANTI 28/04/2026: Automatic control policy (old-project parity).
	//
	// Rules:
	// 1) If a team owns the ball (owner >= 0), that team's controlled player becomes the owner.
	// 2) If a team is defending, controlled player is fixed unless:
	//    - turnover happens (possession changes), or
	//    - user presses switch (I) to toggle nearest/second-nearest defender.
	// 3) Goalkeepers are excluded from defensive switching.
	if (owner >= 0) {
		if (isHomeId(owner)) {
			mControlledHomePlayerId = static_cast<std::uint8_t>(owner);

			// When we gain possession, reset defensive toggle for next time we defend.
			mHomeDefenseSecondClosest = false;
		}
		else {
			mControlledAwayPlayerId = static_cast<std::uint8_t>(owner);

			mAwayDefenseSecondClosest = false;
		}
	}

	const bool turnover =
		(prevPossessingTeamId != -1) &&
		(mPossessingTeamId != -1) &&
		(mPossessingTeamId != prevPossessingTeamId);

	// SANTI 28/04/2026: Defensive control is updated only on turnover or switch press.
	// The "moment of keypress" computation is still done on this tick, but the new ID
	// is used by EngineState on the next tick when building FrameInput.
	const sf::Vector2f ballPos = mWorld.ball().getPosition();

	// Home defending (away possesses).
	if (mPossessingTeamId == 1) {
		int nearest = -1;
		int second = -1;
		findNearestOutfieldDefenders(mWorld, true, ballPos, nearest, second);

		if (turnover) {
			if (nearest >= 0) mControlledHomePlayerId = static_cast<std::uint8_t>(nearest);
			mHomeDefenseSecondClosest = false;
			return;
		}

		if (!homeSwitchPressed) return;

		// Toggle between nearest and second-nearest.
		mHomeDefenseSecondClosest = !mHomeDefenseSecondClosest;
		const int chosen = mHomeDefenseSecondClosest ? second : nearest;
		if (chosen >= 0) mControlledHomePlayerId = static_cast<std::uint8_t>(chosen);
		return;
	}

	// Away defending (home possesses).
	if (mPossessingTeamId == 0) {
		int nearest = -1;
		int second = -1;
		findNearestOutfieldDefenders(mWorld, false, ballPos, nearest, second);

		if (turnover) {
			if (nearest >= 0) mControlledAwayPlayerId = static_cast<std::uint8_t>(nearest);
			mAwayDefenseSecondClosest = false;
			return;
		}

		if (!awaySwitchPressed) return;

		mAwayDefenseSecondClosest = !mAwayDefenseSecondClosest;
		const int chosen = mAwayDefenseSecondClosest ? second : nearest;
		if (chosen >= 0) mControlledAwayPlayerId = static_cast<std::uint8_t>(chosen);
		return;
	}
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
