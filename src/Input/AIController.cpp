#include "AiController.h"
#include "../Common/Constants.h"
#include <algorithm> // SANTI 28/04/2026: std::clamp
#include <cmath>
#include <limits> // SANTI 28/04/2026: std::numeric_limits

namespace {
	static bool isHomeId(std::uint8_t id) {
		return id < 4;
	}

	static float distSq(sf::Vector2f a, sf::Vector2f b) {
		const sf::Vector2f d = a - b;
		return d.x * d.x + d.y * d.y;
	}

	static sf::Vector2f normalizeOrZero(sf::Vector2f v) {
		const float lenSq = v.x * v.x + v.y * v.y;
		const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
		if (lenSq <= eps * eps) return sf::Vector2f(0.f, 0.f);

		const float invLen = 1.f / std::sqrt(lenSq);
		v.x *= invLen;
		v.y *= invLen;
		return v;
	}

	// SANTI 28/04/2026: Returns the nearest opponent distance squared for a given team.
	static float nearestOpponentDistSq(const GameStatePacket& s, bool meIsHome, sf::Vector2f pos) {
		const std::uint8_t start = meIsHome ? 4 : 0;
		const std::uint8_t endInclusive = meIsHome ? 7 : 3;

		float best = std::numeric_limits<float>::max();
		for (std::uint8_t i = start; i <= endInclusive; ++i) {
			const float d2 = distSq(s.players[i].position, pos);
			if (d2 < best) best = d2;
		}
		return best;
	}

	// SANTI 28/04/2026: Returns AI movement speed scale as a fraction of PLAYER_SPEED.
	// Player::applyMoveDirection multiplies moveDirection by PLAYER_SPEED, so scaling
	// here gives AI a different top speed without changing Player.
	static float aiSpeedScale() {
		if (Config::PLAYER_SPEED <= Config::VECTOR_NORMALIZATION_EPSILON) return 1.f;
		return std::clamp(Config::AI_SPEED / Config::PLAYER_SPEED, 0.f, 1.f);
	}

	// SANTI 28/04/2026: Returns the nearest outfield teammate (excluding goalkeeper) to a point.
	static std::uint8_t nearestOutfieldIdTo(
		const GameStatePacket& s,
		bool teamIsHome,
		sf::Vector2f point)
	{
		const std::uint8_t start = teamIsHome ? 0 : 4;
		const std::uint8_t endInclusive = teamIsHome ? 3 : 7;
		const std::uint8_t goalkeeperId = teamIsHome ? 3 : 7;

		std::uint8_t bestId = start;
		float bestD2 = std::numeric_limits<float>::max();

		for (std::uint8_t id = start; id <= endInclusive; ++id) {
			if (id == goalkeeperId) continue;
			const float d2 = distSq(s.players[id].position, point);
			if (d2 >= bestD2) continue;
			bestD2 = d2;
			bestId = id;
		}

		return bestId;
	}
}

InputPacket AIController::getAIInput(std::uint8_t playerID, const GameStatePacket& s, float dt) {
	InputPacket out{};
	out.playerId = playerID;

	if (playerID >= Config::kNumPlayers) return out;

	// SANTI 28/04/2026: Tick down per-player action cooldown.
	// This keeps pass/shoot as impulses, not held buttons, which avoids AI freezing
	// (World uses edge detection on DOWN state).
	if (dt > 0.f) {
		float& cd = mBallActionCooldownSec[playerID];
		cd = std::max(0.f, cd - dt);
	}

	const PlayerState& me = s.players[playerID];

	const bool meIsHome = isHomeId(playerID);
	const float speedScale = aiSpeedScale();

	const int ballOwner = static_cast<int>(s.ballOwnerPlayerId);
	const bool iHaveBall = (ballOwner == static_cast<int>(playerID));

	const int possessingTeamId = static_cast<int>(s.possessingTeamId);
	const int myTeamId = meIsHome ? 0 : 1;
	const bool myTeamPossesses = (possessingTeamId == myTeamId);

	const sf::Vector2f toBall = s.ballPosition - me.position;

	// Goalkeeper: simple "track ball Y" behavior for MVP.
	if (me.isGoalkeeper) {
		// SANTI 28/04/2026: If the goalkeeper has possession, pass immediately.
		// This prevents the match from stalling in singleplayer when the AI keeper saves.
		if (iHaveBall) {
			// SANTI 28/04/2026: One-tick impulse only (cooldown prevents "held" passDown).
			if (mBallActionCooldownSec[playerID] <= 0.f) {
				out.passDown = true;
				mBallActionCooldownSec[playerID] = Config::AI_BALL_ACTION_INTERVAL_SECONDS;
			}

			out.moveDirection = sf::Vector2f(0.f, 0.f);
			return out;
		}

		// Default keeper behavior: track the ball's Y position.
		// Keep keeper movement mostly vertical to feel like a keeper.
		if (toBall.y > Config::VECTOR_NORMALIZATION_EPSILON) {
			out.moveDirection = sf::Vector2f(0.f, 1.f * speedScale);
		}
		else if (toBall.y < -Config::VECTOR_NORMALIZATION_EPSILON) {
			out.moveDirection = sf::Vector2f(0.f, -1.f * speedScale);
		}
		else {
			out.moveDirection = sf::Vector2f(0.f, 0.f);
		}
		return out;
	}

	// ATTACK: if I have possession, decide pass vs shot (no mouse aim).
	if (iHaveBall) {
		// SANTI 28/04/2026: If we're still on cooldown, keep carrying instead of
		// spamming passDown/shootDown (which would be ignored by edge detection).
		const bool canAct = (mBallActionCooldownSec[playerID] <= 0.f);

		const float goalX = meIsHome
			? (Config::RIGHT_GOAL_X + Config::GOAL_WIDTH * 0.5f)
			: (Config::LEFT_GOAL_X + Config::GOAL_WIDTH * 0.5f);

		const float distToGoalX = std::abs(goalX - me.position.x);

		const float pressureDistSq = nearestOpponentDistSq(s, meIsHome, me.position);
		const float pressureDist = std::sqrt(pressureDistSq);

		// SANTI 28/04/2026: Decision rules (MVP, deterministic).
		// 1) If close enough to shoot, shoot.
		// 2) If under pressure, pass.
		// 3) Else carry forward (just move toward goal).
		if (canAct && distToGoalX <= Config::AI_SHOOT_DISTANCE_X) {
			out.shootDown = true;
			out.moveDirection = sf::Vector2f(0.f, 0.f);
			mBallActionCooldownSec[playerID] = Config::AI_BALL_ACTION_INTERVAL_SECONDS;
			return out;
		}

		if (canAct && pressureDist <= Config::AI_PASS_PRESSURE_DISTANCE) {
			out.passDown = true;
			out.moveDirection = sf::Vector2f(0.f, 0.f);
			mBallActionCooldownSec[playerID] = Config::AI_BALL_ACTION_INTERVAL_SECONDS;
			return out;
		}

		// Carry: bias forward + center.
		const float sign = meIsHome ? 1.f : -1.f;
		sf::Vector2f carry(sign, (Config::FIELD_CENTER_Y - me.position.y) * 0.01f);
		out.moveDirection = normalizeOrZero(carry) * speedScale;
		return out;
	}

	// OFF-BALL ATTACK: don't let every AI cluster on the ball.
	// Use a simple "lane + support X" target for each outfield ID.
	if (myTeamPossesses) {
		const std::uint8_t teamStart = meIsHome ? 0 : 4;
		const int outfieldIndex = static_cast<int>(playerID) - static_cast<int>(teamStart); // 0..2 for outfield

		const float laneY = Config::FIELD_CENTER_Y + (outfieldIndex - 1) * Config::AI_LANE_HALFSPACE_Y;
		const float ballInfluenceY = (s.ballPosition.y - Config::FIELD_CENTER_Y) * Config::AI_FORMATION_BALL_INFLUENCE;
		const float targetY = laneY + ballInfluenceY;

		const float sign = meIsHome ? 1.f : -1.f;

		// Support slightly behind the ball so carrier has passing options.
		float targetX = s.ballPosition.x - sign * Config::AI_SUPPORT_OFFSET_X;

		// One off-ball player tends to run ahead to create depth.
		if (outfieldIndex == 2) {
			targetX = s.ballPosition.x + sign * Config::AI_SUPPORT_RUN_AHEAD_X;
		}

		const sf::Vector2f target(targetX, targetY);
		out.moveDirection = normalizeOrZero(target - me.position) * speedScale;
		return out;
	}

	// DEFENSE: only the nearest outfield defender presses the ball.
	// Others "contain" by sitting between ball and own goal in their lane.
	const std::uint8_t nearestDefId = nearestOutfieldIdTo(s, meIsHome, s.ballPosition);
	if (nearestDefId == playerID) {
		out.moveDirection = normalizeOrZero(toBall) * speedScale;
	}
	else {
		const std::uint8_t teamStart = meIsHome ? 0 : 4;
		const int outfieldIndex = static_cast<int>(playerID) - static_cast<int>(teamStart);

		const float laneY = Config::FIELD_CENTER_Y + (outfieldIndex - 1) * Config::AI_LANE_HALFSPACE_Y;
		const float containInfluenceY = (s.ballPosition.y - Config::FIELD_CENTER_Y) * 0.10f;
		const float targetY = laneY + containInfluenceY;

		const float ownGoalX = meIsHome
			? (Config::LEFT_GOAL_X + Config::GOAL_WIDTH)
			: (Config::RIGHT_GOAL_X);

		const float targetX = ownGoalX + (s.ballPosition.x - ownGoalX) * 0.55f;
		const sf::Vector2f target(targetX, targetY);

		out.moveDirection = normalizeOrZero(target - me.position) * speedScale;
	}

	// DEFENSE: attempt tackle if close to the opponent ball owner.
	if (ballOwner >= 0 && ballOwner < static_cast<int>(Config::kNumPlayers)) {
		const bool ownerIsHome = isHomeId(static_cast<std::uint8_t>(ballOwner));
		if (ownerIsHome != meIsHome) {
			const float d2 = distSq(me.position, s.players[ballOwner].position);
			const float r = Config::BALL_STEAL_RADIUS;
			if (d2 <= r * r) {
				out.tackleDown = true;
			}
		}
	}

	return out;
}
