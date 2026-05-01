#include "World.h"

/**
 * @file World.cpp
 * @brief Implementation of the simulation sandbox.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help review small geometry/collision helpers
 * (segment corridors, overlap checks) and to draft some documentation comments. The team implemented
 * the simulation behavior and tuned gameplay through play-testing.
 *
 * Example prompt used:
 * "Review this World simulation file. Suggest small helper functions for geometry/collision checks
 * (e.g., rect overlap, distance squared) and comment the update pipeline for readability, without
 * changing runtime behavior."
 */

#include "../Common/Constants.h"
#include "PhysicsEngine.h"
#include <algorithm>  // std::clamp
#include <array>
#include <cmath>      // std::sqrt
#include <chrono>     // RNG seeding
#include <limits>     // std::numeric_limits
#include <cstdint>    // std::uint32_t

[[maybe_unused]] static bool rectsOverlap(const sf::FloatRect& a, const sf::FloatRect& b) {
	return a.findIntersection(b).has_value(); // SFML 3 replacement for intersects
}

// Convention: player IDs 0-3 are Home, 4-7 are Away.
static bool isHomePlayerId(int playerId) {
	return playerId >= 0 && playerId < 4;
}

// Tiny RNG for chance-based gameplay rules.
// Seeded in World::World() so each run differs, but within a single run the
// behavior is stable (useful for debugging).
static std::uint32_t gWorldRandState = 0xC0FFEEu;
static float rand01() {
	// LCG: fast and good enough for gameplay probability checks.
	gWorldRandState = gWorldRandState * 1664525u + 1013904223u;
	const std::uint32_t mantissa = (gWorldRandState >> 8) & 0x00FFFFFFu;
	return static_cast<float>(mantissa) * (1.0f / 16777216.0f); // [0,1)
}

static float randRange(float minValue, float maxValue) {
	if (maxValue <= minValue) return minValue;
	return minValue + (maxValue - minValue) * rand01();
}

static int randIntInclusive(int minValue, int maxValueInclusive) {
	if (maxValueInclusive <= minValue) return minValue;
	const float t = rand01();
	const int span = (maxValueInclusive - minValue) + 1;
	const int n = static_cast<int>(t * static_cast<float>(span));
	return minValue + std::clamp(n, 0, span - 1);
}

// Returns the closest opponent distance squared to a position.
static float nearestOpponentDistSq(const World& world, bool ownerIsHome, const sf::Vector2f& pos) {
	const std::size_t start = ownerIsHome ? 4 : 0;
	const std::size_t endInclusive = ownerIsHome ? 7 : 3;

	float best = std::numeric_limits<float>::max();
	const auto& ps = world.players();
	for (std::size_t i = start; i <= endInclusive; ++i) {
		const sf::Vector2f d = ps[i].getPosition() - pos;
		const float dsq = d.x * d.x + d.y * d.y;
		if (dsq < best) best = dsq;
	}
	return best;
}

// Returns true if the position is far enough into the attacking direction.
static bool isInFinalThird(const sf::Vector2f& pos, bool attackerIsHome) {
	const float attackRange = Config::PLAYER_MAX_X - Config::PLAYER_MIN_X;
	if (attackRange <= Config::VECTOR_NORMALIZATION_EPSILON) return false;

	const float progress = attackerIsHome
		? (pos.x - Config::PLAYER_MIN_X) / attackRange
		: (Config::PLAYER_MAX_X - pos.x) / attackRange;

	const float clamped = std::clamp(progress, 0.f, 1.f);
	return clamped >= Config::AI_ATTACKING_PROGRESS_FINAL_THIRD;
}

// Minimum distance from a point to a segment.
// Used for shot-lane evaluation (how "blocked" a target lane is).
static float pointToSegmentDistance(
	const sf::Vector2f& point,
	const sf::Vector2f& segmentStart,
	const sf::Vector2f& segmentEnd)
{
	const sf::Vector2f segment = segmentEnd - segmentStart;
	const sf::Vector2f toPoint = point - segmentStart;
	const float lenSq = segment.x * segment.x + segment.y * segment.y;

	if (lenSq <= Config::VECTOR_NORMALIZATION_EPSILON) {
		const sf::Vector2f d = point - segmentStart;
		return std::sqrt(d.x * d.x + d.y * d.y);
	}

	const float t = std::clamp((toPoint.x * segment.x + toPoint.y * segment.y) / lenSq, 0.f, 1.f);
	const sf::Vector2f projection = segmentStart + segment * t;
	const sf::Vector2f d = point - projection;
	return std::sqrt(d.x * d.x + d.y * d.y);
}

// Returns the nearest defender distance to a shot lane.
static float nearestLaneDefenderDistance(
	const World& world,
	bool shooterIsHome,
	const sf::Vector2f& shooterPos,
	const sf::Vector2f& target)
{
	const std::uint8_t defStart = shooterIsHome ? 4 : 0;
	const std::uint8_t defEndInclusive = shooterIsHome ? 7 : 3;

	float best = std::numeric_limits<float>::max();
	const auto& ps = world.players();
	for (std::uint8_t id = defStart; id <= defEndInclusive; ++id) {
		const float d = pointToSegmentDistance(ps[id].getPosition(), shooterPos, target);
		if (d < best) best = d;
	}
	return best;
}

// Choose among center + near-post lanes for a guided shot.
static sf::Vector2f chooseBestShotTarget(const World& world, int shooterId) {
	const bool shooterIsHome = isHomePlayerId(shooterId);
	const sf::Vector2f shooterPos = world.players()[static_cast<std::size_t>(shooterId)].getPosition();

	const float goalX = shooterIsHome
		? (Config::RIGHT_GOAL_X + Config::GOAL_WIDTH * 0.5f)
		: (Config::LEFT_GOAL_X + Config::GOAL_WIDTH * 0.5f);

	const bool inFinalThird = isInFinalThird(shooterPos, shooterIsHome);

	const std::array<sf::Vector2f, 3> targets = {
		sf::Vector2f(goalX, Config::GOAL_CENTER_Y),
		sf::Vector2f(goalX, Config::GOAL_Y_TOP + Config::AI_SHOT_POST_TARGET_MARGIN_Y),
		sf::Vector2f(goalX, Config::GOAL_Y_BOTTOM - Config::AI_SHOT_POST_TARGET_MARGIN_Y)
	};

	sf::Vector2f bestTarget = targets[0];
	float bestScore = -std::numeric_limits<float>::max();

	const float attackSign = shooterIsHome ? 1.f : -1.f;

	for (const auto& target : targets) {
		sf::Vector2f lane = target - shooterPos;
		const float laneLen = std::sqrt(lane.x * lane.x + lane.y * lane.y);
		if (laneLen <= Config::VECTOR_NORMALIZATION_EPSILON) continue;

		lane.x /= laneLen;
		lane.y /= laneLen;

		// Alignment: should be mostly forward (positive dot with attack direction).
		const float alignment = lane.x * attackSign;

		const float nearestDef = nearestLaneDefenderDistance(world, shooterIsHome, shooterPos, target);
		const float laneScore = std::clamp(nearestDef / Config::AI_SHOT_LANE_BLOCK_RADIUS, 0.f, 1.5f);

		const float score =
			alignment * 1.3f +
			laneScore * 1.2f -
			laneLen * 0.001f +
			(inFinalThird ? 0.2f : 0.f);

		if (score <= bestScore) continue;
		bestScore = score;
		bestTarget = target;
	}

	return bestTarget;
}

// Choose a teammate to receive a guided pass.
// We keep this in World.cpp so MatchStates stays easy to read.
//
// Selection goals (MVP):
// - prefer teammates in front of the passer (alignment with attack direction)
// - prefer teammates with more "space" (further from nearest opponent)
// - fall back to nearest teammate if no one is aligned enough
static int chooseBestPassReceiverId(const World& world, int ownerId) {
	if (ownerId < 0) return -1;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return -1;

	const bool ownerIsHome = isHomePlayerId(ownerId);
	const int teamStart = ownerIsHome ? 0 : 4;
	const int teamEndInclusive = ownerIsHome ? 3 : 7;
	const int goalkeeperId = ownerIsHome ? 3 : 7; // never choose GK as a pass target.

	const sf::Vector2f ownerPos = world.players()[ownerId].getPosition();
	const sf::Vector2f attackDir = ownerIsHome ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f);

	// Prefer higher-probability passes over always-forward passes.
	// We score each teammate by:
	// - alignment with attack direction (but allow sideways/back passes under pressure)
	// - available space around the receiver
	// - corridor safety (nearest defender distance to the pass lane)
	const float pressureDist = std::sqrt(nearestOpponentDistSq(world, ownerIsHome, ownerPos));
	const bool underPressure = (pressureDist <= Config::AI_PASS_PRESSURE_DISTANCE);
	const bool inFinalThird = isInFinalThird(ownerPos, ownerIsHome);

	// Start/end for lane safety checks.
	const float sign = ownerIsHome ? 1.f : -1.f;
	const sf::Vector2f offset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);
	const sf::Vector2f startPos = ownerPos + offset;

	// Alignment gate (soft): allow sideways/back passes when they are safer.
	// We only disallow extreme "full backpass" angles to keep passes reasonable.
	float minAlignment = -0.55f;
	if (inFinalThird) minAlignment = -0.70f;
	if (underPressure) minAlignment = -0.85f;

	int bestId = -1;
	float bestScore = -std::numeric_limits<float>::max();

	for (int id = teamStart; id <= teamEndInclusive; ++id) {
		if (id == ownerId) continue;
		if (id == goalkeeperId) continue;

		const sf::Vector2f matePos = world.players()[id].getPosition();
		const sf::Vector2f endPos = matePos + offset;

		sf::Vector2f toMate = matePos - ownerPos;
		const float dist = std::sqrt(toMate.x * toMate.x + toMate.y * toMate.y);
		if (dist <= Config::VECTOR_NORMALIZATION_EPSILON) continue;

		toMate.x /= dist;
		toMate.y /= dist;

		// Alignment with the attack direction (dot with +/-X).
		const float alignment = toMate.x * attackDir.x + toMate.y * attackDir.y;
		if (alignment < minAlignment) continue;

		const float nearestOpp = std::sqrt(nearestOpponentDistSq(world, ownerIsHome, matePos));
		const float spaceScore = std::clamp(nearestOpp / Config::AI_PASS_TARGET_SPACE_BONUS_DISTANCE, 0.f, 1.25f);

		// Lane safety: nearest defender distance to the pass lane.
		// Higher is better. 1.0 means "about corridorRadius away".
		const float laneDefDist = nearestLaneDefenderDistance(world, ownerIsHome, startPos, endPos);
		const float safetyScore = std::clamp(
			laneDefDist / std::max(Config::PASS_INTERCEPTION_CORRIDOR_RADIUS, Config::VECTOR_NORMALIZATION_EPSILON),
			0.f,
			2.0f);

		// Score weights: prioritize safety, then space, then alignment.
		const float alignmentWeight = inFinalThird ? 0.85f : 1.0f;
		const float safetyWeight = 1.75f;
		const float spaceWeight = 1.0f;
		const float distancePenalty = 0.0016f;

		const float score =
			alignment * alignmentWeight +
			spaceScore * spaceWeight +
			safetyScore * safetyWeight -
			dist * distancePenalty;
		if (score <= bestScore) continue;

		bestScore = score;
		bestId = id;
	}

	if (bestId >= 0) return bestId;

	// Fallback: choose the most "safe" lane even if it is sideways/behind.
	int nearestId = -1;
	float bestFallbackScore = -std::numeric_limits<float>::max();
	for (int id = teamStart; id <= teamEndInclusive; ++id) {
		if (id == ownerId) continue;
		if (id == goalkeeperId) continue;

		const sf::Vector2f matePos = world.players()[id].getPosition();
		const sf::Vector2f endPos = matePos + offset;

		const sf::Vector2f d = matePos - ownerPos;
		const float dist = std::sqrt(d.x * d.x + d.y * d.y);
		if (dist <= Config::VECTOR_NORMALIZATION_EPSILON) continue;

		const float nearestOpp = std::sqrt(nearestOpponentDistSq(world, ownerIsHome, matePos));
		const float spaceScore = std::clamp(nearestOpp / Config::AI_PASS_TARGET_SPACE_BONUS_DISTANCE, 0.f, 1.25f);

		const float laneDefDist = nearestLaneDefenderDistance(world, ownerIsHome, startPos, endPos);
		const float safetyScore = std::clamp(
			laneDefDist / std::max(Config::PASS_INTERCEPTION_CORRIDOR_RADIUS, Config::VECTOR_NORMALIZATION_EPSILON),
			0.f,
			2.0f);

		// Prefer safer and closer passes in fallback.
		const float score = safetyScore * 2.0f + spaceScore * 0.8f - dist * 0.0020f;
		if (score <= bestFallbackScore) continue;
		bestFallbackScore = score;
		nearestId = id;
	}

	return nearestId;
}



// ============================================================================
// WORLD IMPLEMENTATION
// ============================================================================
// World owns all game objects and can produce a raw GameStatePacket snapshot
// without owning match logic (score, timer, rules).
// ============================================================================

// ----------------------------------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------------------------------
// Setup pitch bounds and call deterministic kickoff initialization.
World::World()
{
	// Seed chance-based gameplay randomness so it is not the
	// same every time you launch the game.
	//
	// This is safe for networking because the host is authoritative and clients
	// only render snapshots. Determinism across machines is not required.
	const std::uint64_t seed64 =
		static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	gWorldRandState =
		static_cast<std::uint32_t>(seed64) ^
		static_cast<std::uint32_t>(seed64 >> 32) ^
		0xA5A5A5A5u;
	if (gWorldRandState == 0u) gWorldRandState = 1u;

	// Define playable area from (0,0) to (window width, window height).
	mPitchBounds = sf::FloatRect(
		sf::Vector2f(0.f, 0.f),
		sf::Vector2f(
			static_cast<float>(Config::WINDOW_WIDTH),
			static_cast<float>(Config::WINDOW_HEIGHT)
		)
	);

	// Place all objects in their initial kickoff positions.
	resetKickoff();
}

// ----------------------------------------------------------------------------
// DETERMINISTIC KICKOFF RESET
// ----------------------------------------------------------------------------
// Called by constructor and also used when a goal is scored (to restart).
void World::resetKickoff(int kickoffTeamSide)
{

	// Define which goal is left/right once per reset.
	mHomeGoal.setSide(Config::HOME_TEAM_SIDE); // left goal
	mAwayGoal.setSide(Config::AWAY_TEAM_SIDE); // right goal

	// 1) Initialize all 8 players (team assignment, goalkeeper flag).
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const int teamId = (i < 4) ? Config::HOME_TEAM_SIDE : Config::AWAY_TEAM_SIDE;
		const bool isGoalkeeper = (i == 3 || i == 7);   // IDs 3 (home GK) and 7 (away GK)

		mPlayers[i].init(static_cast<int>(i), teamId, isGoalkeeper);

		// Visual properties (optional but helpful for debugging).
		// mPlayers[i].setRadius(Config::PLAYER_HALF_SIZE);
		// mPlayers[i].setOrigin(sf::Vector2f(mPlayers[i].getRadius(), mPlayers[i].getRadius()));
		// mPlayers[i].setFillColor(teamId == 0 ? sf::Color::White : sf::Color(220, 220, 255));
	}

	// 2) Place goalkeepers at their fixed X positions, centered vertically.
	mPlayers[3].setPosition(sf::Vector2f(Config::GOALKEEPER_X_LEFT, Config::FIELD_CENTER_Y));
	mPlayers[7].setPosition(sf::Vector2f(Config::GOALKEEPER_X_RIGHT, Config::FIELD_CENTER_Y));

	// 3) Place outfield players (3 per team) using formation constants.
	//    Home team (indices 0,1,2) on the left, Away team (4,5,6) on the right.
	for (int j = 0; j < 3; ++j) {
		const float yOffset = Config::FORMATION_CENTER_Y + (j - 1) * Config::FORMATION_PLAYER_SPACING_Y;

		// Home outfield players
		mPlayers[j].setPosition(sf::Vector2f(
			Config::FORMATION_HOME_START_X + j * Config::FORMATION_PLAYER_SPACING_X,
			yOffset
		));

		// Away outfield players
		mPlayers[4 + j].setPosition(sf::Vector2f(
			Config::FORMATION_AWAY_START_X + j * Config::FORMATION_PLAYER_SPACING_X,
			yOffset
		));
	}

	// 4) Reset ball to center, clear any owner, stop movement.
	mBall.setPosition(sf::Vector2f(Config::FIELD_CENTER_X, Config::FIELD_CENTER_Y));
	mBall.clearOwner();
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));

	// Restart must cancel any in-flight guided travel state.
	// Without this, a pass/shot that was mid-flight could keep moving the ball
	// after a goal or kickoff reset, because Ball::update() would continue the old travel.
	mBall.cancelGuidedTravel();

	// Clear interaction cooldowns on restart.
	// Kickoff should always be immediately playable.
	mBall.setStealCooldown(0.f);

	// Clear per-player steal retry cooldowns.
	mStealRetryCooldownSec.fill(0.f);

	// Clear input edge history on restart.
	// Kickoff should not "inherit" held buttons from before a goal/state transition.
	mWasPassDown.fill(false);
	mWasShootDown.fill(false);

	// Deterministic kickoff ownership.
	// The kicking team starts with the ball on the center spot.
	// Player IDs are fixed by contract:
	// - Home outfield includes player 0
	// - Away outfield includes player 4
	const int kickoffOwnerId = (kickoffTeamSide == Config::AWAY_TEAM_SIDE) ? 4 : 0;

	// Place the kickoff owner so the ball ends up exactly on the center spot
	// when attached using the X offset.
	const float sign = (kickoffOwnerId < 4) ? 1.f : -1.f;
	mPlayers[kickoffOwnerId].setPosition(sf::Vector2f(
		Config::FIELD_CENTER_X - (Config::BALL_ATTACH_OFFSET_X * sign),
		Config::FIELD_CENTER_Y
	));

	mBall.setOwner(kickoffOwnerId);
	attachBallToOwnerIfAny();

	// IMPORTANT: Cows persist across goals (they do NOT despawn on restart).
	// Resetting cows here would make them "disappear after a goal", which breaks
	// the intended chaos-event fantasy.
}

// ----------------------------------------------------------------------------
// SNAPSHOT PRODUCER (Step 4 deliverable)
// ----------------------------------------------------------------------------
// Fill only the fields that World owns. Does NOT set score, timer, or game state
// (those belong to Match).
void World::writeRawState(GameStatePacket& out) const
{
	// Pitch boundaries (constant per match).
	out.pitchBounds = mPitchBounds;

	// Ball state.
	out.ballPosition = mBall.getPosition();
	out.ballVelocity = mBall.getVelocity();
	out.ballOwnerPlayerId = static_cast<std::int8_t>(mBall.getOwner());

	// Player states (all 8).
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const Player& player = mPlayers[i];
		PlayerState& playerState = out.players[i];

		playerState.position = player.getPosition();
		playerState.velocity = player.getVelocity();
		playerState.facingDirection = player.getFacingDirection();
		playerState.teamId = static_cast<std::uint8_t>(player.getTeam());
		playerState.isGoalkeeper = player.IsGoalkeeper();
		playerState.isLunging = player.IsLunging();
	}

	// Cow snapshot state is always sent (fixed-size), but inactive cows are marked active=false.
	for (std::size_t i = 0; i < Config::kMaxCows; ++i) {
		CowState& cowState = out.cows[i];
		const CowRuntime& cow = mCows[i];

		cowState.active = cow.active;
		cowState.position = cow.position;
		cowState.velocity = cow.velocity;
	}
}


void World::applyFrameMovement(const FrameInput& frameData, float dt) {
	if (dt <= 0.f) return;

	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {

		mPlayers[i].applyMoveDirection(frameData.inputs[i].moveDirection, dt);
	}
}

// ----------------------------------------------------------------------------
// CHAOS EVENT: COW INVASION
// ----------------------------------------------------------------------------
namespace {
	static std::uint8_t oppositeSide(std::uint8_t side) {
		switch (side) {
		case 0: return 1; // left -> right
		case 1: return 0; // right -> left
		case 2: return 3; // top -> bottom
		default: return 2; // bottom -> top
		}
	}

	static sf::Vector2f clampCowInsidePitch(sf::Vector2f pos) {
		const float r = Config::COW_RADIUS;
		pos.x = std::clamp(pos.x, r, static_cast<float>(Config::WINDOW_WIDTH) - r);
		pos.y = std::clamp(pos.y, r, static_cast<float>(Config::WINDOW_HEIGHT) - r);
		return pos;
	}

	// Returns true if the cow center is inside the pitch bounds (accounting for radius).
	// Entering cows start OUTSIDE the pitch, so this helper is used to decide when to clamp.
	static bool isCowCenterInsidePitch(const sf::Vector2f& pos) {
		const float r = Config::COW_RADIUS;
		const float minX = r;
		const float maxX = static_cast<float>(Config::WINDOW_WIDTH) - r;
		const float minY = r;
		const float maxY = static_cast<float>(Config::WINDOW_HEIGHT) - r;
		return (pos.x >= minX && pos.x <= maxX && pos.y >= minY && pos.y <= maxY);
	}

	static bool overlapsGoalMouthExpanded(const sf::Vector2f& pos, float radius, bool& outLeftGoal) {
		// Goal rects are defined in Goal::checkBallCollision and Constants.h.
		// We expand by radius so the cow circle never overlaps the net rectangle.
		const float yMin = Config::GOAL_Y_TOP - radius;
		const float yMax = Config::GOAL_Y_BOTTOM + radius;
		const bool inGoalY = (pos.y >= yMin) && (pos.y <= yMax);
		if (!inGoalY) return false;

		// Left goal: x in [0 .. GOAL_WIDTH]
		const float leftMinX = Config::LEFT_GOAL_X - radius;
		const float leftMaxX = (Config::LEFT_GOAL_X + Config::GOAL_WIDTH) + radius;
		if (pos.x >= leftMinX && pos.x <= leftMaxX) {
			outLeftGoal = true;
			return true;
		}

		// Right goal: x in [RIGHT_GOAL_X .. RIGHT_GOAL_X + GOAL_WIDTH]
		const float rightMinX = Config::RIGHT_GOAL_X - radius;
		const float rightMaxX = (Config::RIGHT_GOAL_X + Config::GOAL_WIDTH) + radius;
		if (pos.x >= rightMinX && pos.x <= rightMaxX) {
			outLeftGoal = false;
			return true;
		}

		return false;
	}

	static void pushCowOutOfGoalMouth(sf::Vector2f& pos) {
		bool isLeftGoal = true;
		// We use an extra clearance margin so cows do not look like they are
		// "inside the goal" even if goal lines/sprites are thicker than GOAL_WIDTH.
		const float expanded = Config::COW_RADIUS + Config::COW_GOAL_MOUTH_CLEARANCE;
		if (!overlapsGoalMouthExpanded(pos, expanded, isLeftGoal)) return;

		// Push horizontally out of the net rectangle.
		if (isLeftGoal) {
			pos.x = (Config::LEFT_GOAL_X + Config::GOAL_WIDTH) + expanded;
		}
		else {
			pos.x = (Config::RIGHT_GOAL_X) - expanded;
		}

		pos = clampCowInsidePitch(pos);
	}

	static sf::Vector2f randomCowTargetInsidePitch() {
		// Stay away from hard edges so cows distribute across the field.
		const float edge = Config::COW_RADIUS + Config::COW_EDGE_CLEARANCE;

		// Expanded goal mouth exclusion radius (for nicer visuals).
		const float expanded = Config::COW_RADIUS + Config::COW_GOAL_MOUTH_CLEARANCE;

		// Try a few samples so cows don't target the goal mouth.
		for (int attempt = 0; attempt < 8; ++attempt) {
			const float x = randRange(
				edge,
				static_cast<float>(Config::WINDOW_WIDTH) - edge);
			const float y = randRange(
				edge,
				static_cast<float>(Config::WINDOW_HEIGHT) - edge);

			sf::Vector2f p(x, y);
			bool isLeftGoal = true;
			if (overlapsGoalMouthExpanded(p, expanded, isLeftGoal)) continue;
			return p;
		}

		// Fallback: center (safe).
		return sf::Vector2f(Config::FIELD_CENTER_X, Config::FIELD_CENTER_Y);
	}

	static sf::Vector2f cowSpawnOutsidePitch(std::uint8_t side) {
		const float margin = Config::COW_RADIUS * 2.f;

		if (side == 0) {
			return sf::Vector2f(
				-margin,
				randRange(Config::COW_RADIUS, static_cast<float>(Config::WINDOW_HEIGHT) - Config::COW_RADIUS));
		}
		if (side == 1) {
			return sf::Vector2f(
				static_cast<float>(Config::WINDOW_WIDTH) + margin,
				randRange(Config::COW_RADIUS, static_cast<float>(Config::WINDOW_HEIGHT) - Config::COW_RADIUS));
		}
		if (side == 2) {
			return sf::Vector2f(
				randRange(Config::COW_RADIUS, static_cast<float>(Config::WINDOW_WIDTH) - Config::COW_RADIUS),
				-margin);
		}
		return sf::Vector2f(
			randRange(Config::COW_RADIUS, static_cast<float>(Config::WINDOW_WIDTH) - Config::COW_RADIUS),
			static_cast<float>(Config::WINDOW_HEIGHT) + margin);
	}

	static sf::Vector2f cowExitTargetOutsidePitch(std::uint8_t side, const sf::Vector2f& fromPos) {
		const float margin = Config::COW_RADIUS * 2.f;

		if (side == 0) return sf::Vector2f(-margin, std::clamp(fromPos.y, Config::COW_RADIUS, static_cast<float>(Config::WINDOW_HEIGHT) - Config::COW_RADIUS));
		if (side == 1) return sf::Vector2f(static_cast<float>(Config::WINDOW_WIDTH) + margin, std::clamp(fromPos.y, Config::COW_RADIUS, static_cast<float>(Config::WINDOW_HEIGHT) - Config::COW_RADIUS));
		if (side == 2) return sf::Vector2f(std::clamp(fromPos.x, Config::COW_RADIUS, static_cast<float>(Config::WINDOW_WIDTH) - Config::COW_RADIUS), -margin);
		return sf::Vector2f(std::clamp(fromPos.x, Config::COW_RADIUS, static_cast<float>(Config::WINDOW_WIDTH) - Config::COW_RADIUS), static_cast<float>(Config::WINDOW_HEIGHT) + margin);
	}

	static bool isCowOutsidePitch(const sf::Vector2f& pos) {
		const float margin = Config::COW_RADIUS * 1.5f;
		if (pos.x < -margin) return true;
		if (pos.x > static_cast<float>(Config::WINDOW_WIDTH) + margin) return true;
		if (pos.y < -margin) return true;
		if (pos.y > static_cast<float>(Config::WINDOW_HEIGHT) + margin) return true;
		return false;
	}

	static float dot(sf::Vector2f a, sf::Vector2f b) {
		return a.x * b.x + a.y * b.y;
	}

	// Segment-circle intersection (earliest t in [0,1]). Used for ball-cow blocking so the
	// ball doesn't tunnel through cows at high speed.
	static bool segmentCircleHit(
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		const sf::Vector2f& center,
		float radius,
		float& outT,
		sf::Vector2f& outPoint,
		sf::Vector2f& outNormal)
	{
		const sf::Vector2f d = end - start;
		const sf::Vector2f f = start - center;

		const float a = dot(d, d);
		if (a <= Config::VECTOR_NORMALIZATION_EPSILON) {
			const sf::Vector2f delta = start - center;
			const float distSq = dot(delta, delta);
			if (distSq > radius * radius) return false;

			outT = 0.f;
			outPoint = start;
			const float dist = std::sqrt(std::max(distSq, Config::VECTOR_NORMALIZATION_EPSILON));
			outNormal = (dist > 0.f) ? (delta * (1.f / dist)) : sf::Vector2f(1.f, 0.f);
			return true;
		}

		const float b = 2.f * dot(f, d);
		const float c = dot(f, f) - radius * radius;
		const float disc = b * b - 4.f * a * c;
		if (disc < 0.f) return false;

		const float sqrtDisc = std::sqrt(disc);
		const float inv2a = 1.f / (2.f * a);

		const float t1 = (-b - sqrtDisc) * inv2a;
		const float t2 = (-b + sqrtDisc) * inv2a;

		float tHit = 2.f;
		if (t1 >= 0.f && t1 <= 1.f) tHit = t1;
		else if (t2 >= 0.f && t2 <= 1.f) tHit = t2;

		if (tHit > 1.f) return false;

		outT = tHit;
		outPoint = start + d * tHit;

		sf::Vector2f n = outPoint - center;
		const float nLenSq = dot(n, n);
		if (nLenSq <= Config::VECTOR_NORMALIZATION_EPSILON) {
			n = sf::Vector2f(1.f, 0.f);
		}
		else {
			const float invLen = 1.f / std::sqrt(nLenSq);
			n.x *= invLen;
			n.y *= invLen;
		}
		outNormal = n;
		return true;
	}
}

void World::updateCows(float dt) {
	// Called only by the host simulation (PlayingState). The client never calls this.
	if (dt <= 0.f) return;

	// New design (per your feedback):
	// - Cows spawn over time until kMaxCows is reached.
	// - Cows persist across goals (World::resetKickoff does not clear them).
	// - Cows never exit; they wander and pause until match end.

	// 1) Spawn scheduling: if we have room for more cows, tick down to next spawn.
	bool hasFreeSlot = false;
	for (const CowRuntime& cow : mCows) {
		if (!cow.active) { hasFreeSlot = true; break; }
	}

	if (hasFreeSlot) {
		if (mCowNextSpawnCountdownSec <= 0.f) {
			mCowNextSpawnCountdownSec = randRange(
				Config::COW_SPAWN_MIN_DELAY_SECONDS,
				Config::COW_SPAWN_MAX_DELAY_SECONDS);
		}

		mCowNextSpawnCountdownSec = std::max(0.f, mCowNextSpawnCountdownSec - dt);

		if (mCowNextSpawnCountdownSec <= 0.f) {
			// Spawn exactly ONE new cow (simpler, more readable, better pacing).
			for (CowRuntime& cow : mCows) {
				if (cow.active) continue;

				cow = CowRuntime{};
				cow.active = true;
				cow.phase = 0; // entering
				cow.entrySide = static_cast<std::uint8_t>(randIntInclusive(0, 3));
				cow.position = cowSpawnOutsidePitch(cow.entrySide);
				cow.target = randomCowTargetInsidePitch();
				cow.velocity = sf::Vector2f(0.f, 0.f);
				break;
			}

			// Schedule the next spawn.
			mCowNextSpawnCountdownSec = randRange(
				Config::COW_SPAWN_MIN_DELAY_SECONDS,
				Config::COW_SPAWN_MAX_DELAY_SECONDS);
		}
	}

	// 2) Update each active cow.
	for (CowRuntime& cow : mCows) {
		if (!cow.active) continue;

		const float eps = Config::COW_TARGET_REACHED_EPSILON;
		const float epsSq = eps * eps;

		// Pause phase: stand still, then pick a new random target.
		if (cow.phase == 2) {
			cow.velocity = sf::Vector2f(0.f, 0.f);
			cow.phaseTimerSec = std::max(0.f, cow.phaseTimerSec - dt);

			if (cow.phaseTimerSec <= 0.f) {
				cow.phase = 1; // moving
				cow.target = randomCowTargetInsidePitch();
				cow.phaseTimerSec = randRange(
					Config::COW_WANDER_MOVE_MIN_SECONDS,
					Config::COW_WANDER_MOVE_MAX_SECONDS);
			}

			// Keep paused cows in legal space too.
			if (isCowCenterInsidePitch(cow.position)) {
				cow.position = clampCowInsidePitch(cow.position);
				pushCowOutOfGoalMouth(cow.position);
			}

			continue;
		}

		// Entering + moving share the same movement code path.
		float speed = (cow.phase == 0) ? Config::COW_SPEED_ENTERING : Config::COW_SPEED_GRAZING;
		if (cow.phase == 1) {
			// Time-limited movement: after this timer runs out, the cow pauses.
			cow.phaseTimerSec = std::max(0.f, cow.phaseTimerSec - dt);
		}

		sf::Vector2f dir = cow.target - cow.position;
		const float lenSq = dir.x * dir.x + dir.y * dir.y;

		if (lenSq <= Config::VECTOR_NORMALIZATION_EPSILON) {
			cow.velocity = sf::Vector2f(0.f, 0.f);
		}
		else {
			const float invLen = 1.f / std::sqrt(lenSq);
			dir.x *= invLen;
			dir.y *= invLen;
			cow.velocity = dir * speed;
		}

		cow.position += cow.velocity * dt;

		// Only clamp once the cow is inside the pitch. (Entering cows start outside.)
		if (isCowCenterInsidePitch(cow.position)) {
			cow.position = clampCowInsidePitch(cow.position);
			pushCowOutOfGoalMouth(cow.position);
		}

		const sf::Vector2f toTarget = cow.target - cow.position;
		const float distSq = toTarget.x * toTarget.x + toTarget.y * toTarget.y;
		const bool reachedTarget = (distSq <= epsSq);

		// Phase transitions.
		if (cow.phase == 0) {
			// Entering -> pause after reaching the first target inside the pitch.
			if (reachedTarget && isCowCenterInsidePitch(cow.position)) {
				cow.phase = 2; // paused
				cow.phaseTimerSec = randRange(
					Config::COW_WANDER_PAUSE_MIN_SECONDS,
					Config::COW_WANDER_PAUSE_MAX_SECONDS);
				cow.velocity = sf::Vector2f(0.f, 0.f);
			}
		}
		else if (cow.phase == 1) {
			// Moving -> pause when timer expires or the target is reached.
			if (reachedTarget || cow.phaseTimerSec <= 0.f) {
				cow.phase = 2; // paused
				cow.phaseTimerSec = randRange(
					Config::COW_WANDER_PAUSE_MIN_SECONDS,
					Config::COW_WANDER_PAUSE_MAX_SECONDS);
				cow.velocity = sf::Vector2f(0.f, 0.f);
			}
		}
	}
}

void World::resolveCowPlayerCollisions(float dt) {
	// Cows are heavy obstacles: we only push PLAYERS out (not cows).
	(void)dt;

	const float minDist = Config::COW_RADIUS + Config::PLAYER_HALF_SIZE;
	const float minDistSq = minDist * minDist;

	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	const float epsSq = eps * eps;

	for (const CowRuntime& cow : mCows) {
		if (!cow.active) continue;

		for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
			sf::Vector2f p = mPlayers[i].getPosition();
			const sf::Vector2f delta = p - cow.position;
			const float dSq = delta.x * delta.x + delta.y * delta.y;
			if (dSq >= minDistSq) continue;
			if (dSq <= epsSq) continue;

			const float d = std::sqrt(dSq);
			const sf::Vector2f n(delta.x / d, delta.y / d);
			p = cow.position + n * minDist;

			// Apply the same bounds rules used elsewhere (goalkeepers are special).
			if (mPlayers[i].IsGoalkeeper()) {
				const bool isHomeKeeper = (mPlayers[i].getTeam() == Config::HOME_TEAM_SIDE);
				p.x = isHomeKeeper ? Config::GOALKEEPER_X_LEFT : Config::GOALKEEPER_X_RIGHT;

				const float yMin = Config::GOAL_Y_TOP + Config::PLAYER_HALF_SIZE;
				const float yMax = Config::GOAL_Y_BOTTOM - Config::PLAYER_HALF_SIZE;
				p.y = std::clamp(p.y, yMin, yMax);
			}
			else {
				p.x = std::clamp(p.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
				p.y = std::clamp(p.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
			}

			mPlayers[i].setPosition(p);
		}
	}
}

void World::resolveCowBallCollisions(float dt, const sf::Vector2f& prevBallPos) {
	// Ball is blocked by cows even during guided travel.
	if (dt <= 0.f) return;
	if (mBall.getOwner() >= 0) return;

	// No cows active: nothing to do.
	bool anyCow = false;
	for (const CowRuntime& cow : mCows) {
		if (cow.active) { anyCow = true; break; }
	}
	if (!anyCow) return;

	const sf::Vector2f ballNow = mBall.getPosition();
	const float radius = Config::COW_RADIUS + Config::BALL_RADIUS;

	float bestT = 2.f;
	sf::Vector2f bestPoint{ 0.f, 0.f };
	sf::Vector2f bestNormal{ 1.f, 0.f };

	for (const CowRuntime& cow : mCows) {
		if (!cow.active) continue;

		float t = 0.f;
		sf::Vector2f p{ 0.f, 0.f };
		sf::Vector2f n{ 1.f, 0.f };
		if (!segmentCircleHit(prevBallPos, ballNow, cow.position, radius, t, p, n)) continue;
		if (t >= bestT) continue;

		bestT = t;
		bestPoint = p;
		bestNormal = n;
	}

	if (bestT > 1.f) return;

	// Compute effective incoming velocity (works for both guided and velocity motion).
	const sf::Vector2f incoming = (ballNow - prevBallPos) * (1.f / dt);

	// Reflect across the collision normal: v' = v - 2*(v dot n)*n
	const float vn = incoming.x * bestNormal.x + incoming.y * bestNormal.y;
	sf::Vector2f reflected = incoming - bestNormal * (2.f * vn);
	reflected *= Config::COW_BALL_BOUNCE_DAMPING;

	const float speedSq = reflected.x * reflected.x + reflected.y * reflected.y;
	if (speedSq <= Config::COW_BALL_MIN_BOUNCE_SPEED * Config::COW_BALL_MIN_BOUNCE_SPEED) {
		// If the bounce is too small, push the ball away so it doesn't "stick" to cows.
		reflected = bestNormal * Config::COW_BALL_MIN_BOUNCE_SPEED;
	}

	// Cancel guided travel so the ball becomes physics-driven after the block.
	mBall.cancelGuidedTravel();

	// Place ball just outside the cow circle.
	sf::Vector2f correctedPos = bestPoint + bestNormal * (Config::VECTOR_NORMALIZATION_EPSILON * 4.f);

	// Clamp to pitch bounds for safety.
	const float r = Config::BALL_RADIUS;
	correctedPos.x = std::clamp(correctedPos.x, r, static_cast<float>(Config::WINDOW_WIDTH) - r);
	correctedPos.y = std::clamp(correctedPos.y, r, static_cast<float>(Config::WINDOW_HEIGHT) - r);

	mBall.setPosition(correctedPos);
	mBall.setVelocity(reflected);

	// Allow pickup soon after the deflection.
	mBall.setStealCooldown(Config::POST_KICK_PICKUP_DELAY_SECONDS);
}

void World::attachBallToOwnerIfAny() {
	// MatchStates decide WHEN to call this (usually every tick during Playing).
	const int ownerId = mBall.getOwner();
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;

	// For now, attach the ball slightly in front of the owner along the X axis.
	// Home team (0-3) attacks to the right, Away team (4-7) attacks to the left.
	const float sign = (ownerId < 4) ? 1.f : -1.f;
	const sf::Vector2f offset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f ownerPos = mPlayers[ownerId].getPosition();
	mBall.setPosition(ownerPos + offset);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));
}

void World::tryPickupLooseBall(float pickupRadius) {
	// This assigns ball ownership to the closest player inside pickupRadius.
	if (pickupRadius <= 0.f) return;

	if (mBall.getOwner() >= 0) return;

	// Use Ball's cooldown as a "pickup grace timer" after kicks.
	// This prevents immediate re-ownership on the same frame a kick happens.
	if (mBall.getStealCooldown() > 0.f) return;

	const sf::Vector2f ballPos = mBall.getPosition();
	const float pickupRadiusSq = pickupRadius * pickupRadius;

	int bestId = -1;
	float bestDistSq = std::numeric_limits<float>::max();

	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const sf::Vector2f p = mPlayers[i].getPosition();
		const sf::Vector2f d = p - ballPos;
		const float distSq = d.x * d.x + d.y * d.y;

		if (distSq >= bestDistSq) continue;
		bestDistSq = distSq;
		bestId = static_cast<int>(i);
	}

	if (bestId < 0) return;
	if (bestDistSq > pickupRadiusSq) return;

	mBall.setOwner(bestId);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));
}


void World::resolveTackleSteals(const FrameInput& frameData, float dt) {
	// Simplified, deterministic steal/tackle resolution.
	// This intentionally avoids complex probabilistic outcomes:
	// - No randomness (always succeeds if in range and cooldown allows).
	// - Uses a fixed retry cooldown per challenger to avoid spam.
	if (dt <= 0.f) return;

	// Tick retry cooldowns.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		if (mStealRetryCooldownSec[i] <= 0.f) continue;
		mStealRetryCooldownSec[i] = std::max(0.f, mStealRetryCooldownSec[i] - dt);
	}

	const int ownerId = mBall.getOwner();
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;

	// Guard: do not allow steals while the ball is protected.
	if (mBall.getStealCooldown() > 0.f) return;

	const bool ownerIsHome = isHomePlayerId(ownerId);
	const sf::Vector2f ownerPos = mPlayers[ownerId].getPosition();

	int bestStealerId = -1;
	float bestDistSq = std::numeric_limits<float>::max();

	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const int challengerId = static_cast<int>(i);
		if (challengerId == ownerId) continue;

		// Only opponents can steal.
		const bool challengerIsHome = isHomePlayerId(challengerId);
		if (challengerIsHome == ownerIsHome) continue;

		// Retry cooldown prevents "held tackle" from stealing every frame.
		if (mStealRetryCooldownSec[i] > 0.f) continue;

		// Only attempt if this challenger is actually trying to tackle this frame.
		if (!frameData.inputs[i].tackleDown) continue;

		// Humans (and any AI using tackleDown) get a larger tackle radius.
		const float radius = Config::HUMAN_MANUAL_TACKLE_RADIUS;
		const float radiusSq = radius * radius;

		const sf::Vector2f d = mPlayers[i].getPosition() - ownerPos;
		const float distSq = d.x * d.x + d.y * d.y;
		if (distSq > radiusSq) continue;

		if (distSq >= bestDistSq) continue;
		bestDistSq = distSq;
		bestStealerId = challengerId;
	}

	if (bestStealerId < 0) return;

	// Success: transfer ownership immediately.
	mBall.setOwner(bestStealerId);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));

	// Small cooldown to prevent instant ping-pong steals.
	mBall.setStealCooldown(Config::BALL_STEAL_COOLDOWN_SECONDS);

	// Challenger cannot steal again immediately even if ball becomes loose.
	mStealRetryCooldownSec[static_cast<std::size_t>(bestStealerId)] = Config::STEALER_RETRY_COOLDOWN_SECONDS;
}

void World::enforceGoalkeeperSixYardBoxProtection() {
	// If a goalkeeper is holding the ball, prevent opponents
	// and teammates from entering the keeper's six-yard box.
	//
	// This is a gameplay rule enforced by the host simulation (authoritative).
	// It is not a physics query and should not live inside PhysicsEngine.
	const int ownerId = mBall.getOwner();
	const bool ownerIsGoalkeeper = (ownerId == 3 || ownerId == 7);
	if (!ownerIsGoalkeeper) return;

	// If a pass/shot is currently in guided flight, no one "holds" the ball.
	if (mBall.isGuidedInFlight()) return;

	const bool keeperIsHome = isHomePlayerId(ownerId);
	const float keeperX = mPlayers[ownerId].getPosition().x;

	// Box bounds are defined in Constants.h. We use "player center" coordinates
	// (because Player origin is the center of the circle).
	const float yMin = std::clamp(
		Config::GOAL_Y_TOP - Config::SIX_YARD_BOX_MARGIN_Y,
		Config::PLAYER_MIN_Y,
		Config::PLAYER_MAX_Y);
	const float yMax = std::clamp(
		Config::GOAL_Y_BOTTOM + Config::SIX_YARD_BOX_MARGIN_Y,
		Config::PLAYER_MIN_Y,
		Config::PLAYER_MAX_Y);

	float xMin = 0.f;
	float xMax = 0.f;
	if (keeperIsHome) {
		// Home goalkeeper (id 3) defends the left goal at x ~= 0.
		xMin = Config::PLAYER_MIN_X;
		xMax = std::clamp(
			Config::LEFT_GOAL_X + Config::SIX_YARD_BOX_DEPTH,
			Config::PLAYER_MIN_X,
			Config::PLAYER_MAX_X);
	}
	else {
		// Away goalkeeper (id 7) defends the right goal at x ~= WINDOW_WIDTH.
		const float goalLineX = Config::RIGHT_GOAL_X + Config::GOAL_WIDTH;
		xMin = std::clamp(
			goalLineX - Config::SIX_YARD_BOX_DEPTH,
			Config::PLAYER_MIN_X,
			Config::PLAYER_MAX_X);
		xMax = Config::PLAYER_MAX_X;
	}

	// Push players out horizontally. This is intentionally simple and stable.
	// It avoids jitter from trying to compute a "minimum translation vector".
	for (std::uint8_t id = 0; id < static_cast<std::uint8_t>(Config::kNumPlayers); ++id) {
		if (static_cast<int>(id) == ownerId) continue;

		sf::Vector2f pos = mPlayers[id].getPosition();
		bool changed = false;

		// Extra safety rule:
		// On our small top-down pitch there is in-play space "behind" the keeper
		// (between the keeper's fixed X and the goal line). That can cause
		// football-nonsense situations where players drift behind their own keeper
		// even if they are not inside the six-yard box Y-range.
		//
		// While the keeper is HOLDING the ball, we ensure nobody can be behind the
		// keeper line for ANY Y. This applies to BOTH teams and both teammate/opponent.
		if (keeperIsHome) {
			const float minAllowedX = keeperX + Config::PLAYER_HALF_SIZE;
			if (pos.x < minAllowedX) {
				pos.x = minAllowedX;
				changed = true;
			}
		}
		else {
			const float maxAllowedX = keeperX - Config::PLAYER_HALF_SIZE;
			if (pos.x > maxAllowedX) {
				pos.x = maxAllowedX;
				changed = true;
			}
		}

		const bool inside =
			(pos.x >= xMin && pos.x <= xMax) &&
			(pos.y >= yMin && pos.y <= yMax);
		if (inside) {
			if (keeperIsHome) {
				pos.x = xMax + Config::PLAYER_HALF_SIZE;
			}
			else {
				pos.x = xMin - Config::PLAYER_HALF_SIZE;
			}
			changed = true;
		}

		if (!changed) continue;

		pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
		pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
		mPlayers[id].setPosition(pos);
	}
}

void World::enforceNoPlayersInsideGoalMouth() {
	// Hard gameplay rule for immersion:
	// No player should ever stand inside the goal mouth rectangle (the net area).
	//
	// On a small pitch, separation pushes and simple AI steering can drift players
	// into the goal itself, including teammates behind their own goalkeeper.
	// Real football never has players standing inside the net, so we enforce this
	// correction for BOTH goals, every tick.
	//
	// Implementation: if a player's center point is inside the goal-mouth region,
	// push them out along X to the nearest valid edge. Keep it simple to avoid jitter.
	const float paddedYMin = Config::GOAL_Y_TOP - Config::PLAYER_HALF_SIZE;
	const float paddedYMax = Config::GOAL_Y_BOTTOM + Config::PLAYER_HALF_SIZE;

	const float leftGoalMaxX = Config::LEFT_GOAL_X + Config::GOAL_WIDTH + Config::PLAYER_HALF_SIZE;
	const float rightGoalMinX = Config::RIGHT_GOAL_X - Config::PLAYER_HALF_SIZE;

	for (std::uint8_t id = 0; id < static_cast<std::uint8_t>(Config::kNumPlayers); ++id) {
		sf::Vector2f pos = mPlayers[id].getPosition();

		const bool inGoalY = (pos.y >= paddedYMin && pos.y <= paddedYMax);
		if (!inGoalY) continue;

		if (pos.x <= leftGoalMaxX) {
			pos.x = leftGoalMaxX;
			pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
			pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
			mPlayers[id].setPosition(pos);
			continue;
		}

		if (pos.x >= rightGoalMinX) {
			pos.x = rightGoalMinX;
			pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
			pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
			mPlayers[id].setPosition(pos);
			continue;
		}
	}
}

void World::kickGuaranteedPassWithInterception(int ownerId) {
	// A guided pass is initiated by input, but World owns the
	// mechanics (target choice + interception query + ball kick).
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;
	if (mBall.getOwner() != ownerId) return;

	// Don't start a new pass if one is already in flight.
	if (mBall.isGuidedInFlight()) return;

	const bool ownerIsHome = isHomePlayerId(ownerId);
	const float sign = ownerIsHome ? 1.f : -1.f;
	const sf::Vector2f ownerOffset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f startPos = mPlayers[ownerId].getPosition() + ownerOffset;

	const int receiverId = chooseBestPassReceiverId(*this, ownerId);
	if (receiverId < 0) return;

	// Pass to a fixed endpoint (target's current position). This avoids "homing passes"
	// and keeps behavior stable frame-to-frame.
	const sf::Vector2f receiverPos = mPlayers[receiverId].getPosition();
	const sf::Vector2f endPos = receiverPos + ownerOffset;

	// Defenders are the opposite side IDs (no Team class).
	const std::uint8_t defStart = ownerIsHome ? 4 : 0;
	const std::uint8_t defEndInclusive = ownerIsHome ? 7 : 3;

	const SegmentInterceptionResult intercept =
		PhysicsEngine::findFirstSegmentInterception(
			*this,
			startPos,
			endPos,
			defStart,
			defEndInclusive,
			Config::PASS_INTERCEPTION_CORRIDOR_RADIUS);

	const bool intercepted = intercept.hasInterception && intercept.interceptorId != 255;
	const sf::Vector2f finalEnd = intercepted ? intercept.interceptPoint : endPos;
	const int finalOwnerId = intercepted ? static_cast<int>(intercept.interceptorId) : receiverId;

	// Prepare ball for flight.
	mBall.setPosition(startPos);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));

	// Guided travel: no pickup/possession change until it resolves.
	mBall.beginGuidedTravel(finalEnd, Config::GUARANTEED_PASS_SPEED, finalOwnerId);
}

void World::kickGuaranteedShotWithInterception(int ownerId) {
	// A guided shot aims at the opponent goal, but defenders can
	// intercept along a corridor before the ball reaches the goal.
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;
	if (mBall.getOwner() != ownerId) return;

	const bool ownerIsHome = isHomePlayerId(ownerId);

	// Don't start a new shot if one is already in flight.
	if (mBall.isGuidedInFlight()) return;

	const float sign = ownerIsHome ? 1.f : -1.f;
	const sf::Vector2f ownerOffset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f startPos = mPlayers[ownerId].getPosition() + ownerOffset;

	// Lane selection (center vs near-post) based on defender spacing.
	const sf::Vector2f intendedEnd = chooseBestShotTarget(*this, ownerId);

	const std::uint8_t defStart = ownerIsHome ? 4 : 0;
	const std::uint8_t defEndInclusive = ownerIsHome ? 7 : 3;

	const SegmentInterceptionResult intercept =
		PhysicsEngine::findFirstSegmentInterception(
			*this,
			startPos,
			intendedEnd,
			defStart,
			defEndInclusive,
			Config::SHOT_INTERCEPTION_CORRIDOR_RADIUS);

	const bool intercepted = intercept.hasInterception && intercept.interceptorId != 255;

	sf::Vector2f finalEnd = intercepted ? intercept.interceptPoint : intendedEnd;

	// If intercepted, defender gains possession at the end.
	// If not intercepted, we apply a distance-based goalkeeper save chance.
	int finalOwnerId = intercepted ? static_cast<int>(intercept.interceptorId) : -1;

	if (!intercepted) {
		// Distance-based save probability.
		// Farther shots are easier to save; closer shots are harder.
		const float distX = std::abs(intendedEnd.x - startPos.x);

		const float denom = std::max(
			Config::SHOT_SAVE_DISTANCE_FAR_X - Config::SHOT_SAVE_DISTANCE_CLOSE_X,
			Config::VECTOR_NORMALIZATION_EPSILON);
		const float t = std::clamp(
			(distX - Config::SHOT_SAVE_DISTANCE_CLOSE_X) / denom,
			0.f,
			1.f);

		const float saveChance =
			Config::SHOT_SAVE_CHANCE_CLOSE +
			(Config::SHOT_SAVE_CHANCE_FAR - Config::SHOT_SAVE_CHANCE_CLOSE) * t;

		// Defending goalkeeper id is fixed by team side.
		const int goalkeeperId = ownerIsHome ? 7 : 3;

		if (rand01() < saveChance) {
			// Saved: end the shot at the goalkeeper position so it doesn't count as a goal.
			finalEnd = mPlayers[goalkeeperId].getPosition();
			finalOwnerId = goalkeeperId;
		}
	}

	mBall.setPosition(startPos);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));
	mBall.beginGuidedTravel(finalEnd, Config::GUARANTEED_SHOT_SPEED_MAX, finalOwnerId);
}

bool World::isPassPressed(const FrameInput& frameData, int playerId) const {
	// Edge detection for passDown (J key).
	// "Pressed" means down now, but was not down last tick.
	if (playerId < 0) return false;
	if (playerId >= static_cast<int>(Config::kNumPlayers)) return false;

	const std::size_t idx = static_cast<std::size_t>(playerId);
	const bool down = frameData.inputs[idx].passDown;
	return down && !mWasPassDown[idx];
}

bool World::isShootPressed(const FrameInput& frameData, int playerId) const {
	// Edge detection for shootDown (K key).
	if (playerId < 0) return false;
	if (playerId >= static_cast<int>(Config::kNumPlayers)) return false;

	const std::size_t idx = static_cast<std::size_t>(playerId);
	const bool down = frameData.inputs[idx].shootDown;
	return down && !mWasShootDown[idx];
}

void World::commitActionButtonHistory(const FrameInput& frameData) {
	// Commit DOWN-state history for edge detection.
	// Must be called once per simulation tick (during Playing) after you've read edges.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		mWasPassDown[i] = frameData.inputs[i].passDown;
		mWasShootDown[i] = frameData.inputs[i].shootDown;
	}
}

// ----------------------------------------------------------------------------
// KICKOFF RULES
// ----------------------------------------------------------------------------
void World::enforceKickoffDefenderRestrictions(int kickoffTeamSide) {
	const float centerX = Config::FIELD_CENTER_X;
	const float centerY = Config::FIELD_CENTER_Y;

	// Kickoff owner is the fixed center player for the kicking team.
	// Home kickoff: player 0. Away kickoff: player 4.
	const int kickoffOwnerId = (kickoffTeamSide == Config::AWAY_TEAM_SIDE) ? 4 : 0;

	// Keep non-kickers outside the circle.
	const float radiusWanted = Config::KICKOFF_CIRCLE_RADIUS + Config::PLAYER_HALF_SIZE;
	const float radiusWantedSq = radiusWanted * radiusWanted;

	const float halfLineXMin = centerX + Config::PLAYER_HALF_SIZE;
	const float halfLineXMax = centerX - Config::PLAYER_HALF_SIZE;

	for (int id = 0; id < static_cast<int>(Config::kNumPlayers); ++id) {
		if (id == kickoffOwnerId) continue;

		sf::Vector2f pos = mPlayers[id].getPosition();

		// 1) Enforce half: everyone except the kickoff taker stays in their own half.
		// Home players (0-3) stay on the left half, Away players (4-7) stay on the right half.
		const int teamSide = (id < 4) ? Config::HOME_TEAM_SIDE : Config::AWAY_TEAM_SIDE;
		if (teamSide == Config::AWAY_TEAM_SIDE) {
			pos.x = std::max(pos.x, halfLineXMin);
		}
		else {
			pos.x = std::min(pos.x, halfLineXMax);
		}

		// 2) Enforce kickoff circle: everyone except the kickoff taker stays outside.
		const float dx = pos.x - centerX;
		const float dy = pos.y - centerY;
		const float d2 = dx * dx + dy * dy;
		if (d2 < radiusWantedSq) {
			const float sign = (teamSide == Config::AWAY_TEAM_SIDE) ? 1.f : -1.f;
			pos.x = centerX + sign * radiusWanted;
		}

		// 3) Clamp to pitch bounds.
		pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
		pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
		mPlayers[id].setPosition(pos);
	}
}

void World::kickKickoffPassToTeammate(int kickoffOwnerId) {
	// Kickoff pass should be a clean "opening pass" before
	// we allow open play. Do not allow interceptions here.
	if (kickoffOwnerId < 0) return;
	if (kickoffOwnerId >= static_cast<int>(Config::kNumPlayers)) return;
	if (mBall.getOwner() != kickoffOwnerId) return;

	// Do not start another kickoff pass if one is already in flight.
	if (mBall.isGuidedInFlight()) return;

	const int receiverId = chooseBestPassReceiverId(*this, kickoffOwnerId);
	if (receiverId < 0) return;

	const float sign = (kickoffOwnerId < 4) ? 1.f : -1.f;
	const sf::Vector2f offset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f startPos = mPlayers[kickoffOwnerId].getPosition() + offset;
	const sf::Vector2f endPos = mPlayers[receiverId].getPosition() + offset;

	mBall.setPosition(startPos);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));

	// Guided travel means no possession change until the pass resolves.
	mBall.beginGuidedTravel(endPos, Config::GUARANTEED_PASS_SPEED, receiverId);
}
