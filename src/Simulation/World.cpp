#include "World.h"
#include "../Common/Constants.h"
#include "PhysicsEngine.h" // SANTI: interception query for guaranteed pass/shot
#include <algorithm>  // SANTI: std::clamp
#include <array>      // SANTI 28/04/2026: fixed shot lane candidates
#include <cmath>      // SANTI 28/04/2026: std::sqrt
#include <limits>     // SANTI: std::numeric_limits

[[maybe_unused]] static bool rectsOverlap(const sf::FloatRect& a, const sf::FloatRect& b) {
	return a.findIntersection(b).has_value(); // SFML 3 replacement for intersects
}

// SANTI: Convention: player IDs 0-3 are Home, 4-7 are Away.
static bool isHomePlayerId(int playerId) {
	return playerId >= 0 && playerId < 4;
}

// SANTI 28/04/2026: Returns the closest opponent distance squared to a position.
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

// SANTI 28/04/2026: "Final third" check (old project parity).
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

// SANTI 28/04/2026: Minimum distance from a point to a segment.
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

// SANTI 28/04/2026: Returns the nearest defender distance to a shot lane.
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

// SANTI 28/04/2026: Choose among center + near-post lanes for a guaranteed shot.
// This is the Team-less port of old ShotTargetingService.
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

// SANTI 28/04/2026: Chooses a teammate to receive a "guaranteed pass".
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
	const int goalkeeperId = ownerIsHome ? 3 : 7; // SANTI 28/04/2026: never choose GK as a pass target.

	const sf::Vector2f ownerPos = world.players()[ownerId].getPosition();
	const sf::Vector2f attackDir = ownerIsHome ? sf::Vector2f(1.f, 0.f) : sf::Vector2f(-1.f, 0.f);

	int bestId = -1;
	float bestScore = -std::numeric_limits<float>::max();

	for (int id = teamStart; id <= teamEndInclusive; ++id) {
		if (id == ownerId) continue;
		if (id == goalkeeperId) continue; // SANTI 28/04/2026

		const sf::Vector2f matePos = world.players()[id].getPosition();
		sf::Vector2f toMate = matePos - ownerPos;
		const float dist = std::sqrt(toMate.x * toMate.x + toMate.y * toMate.y);
		if (dist <= Config::VECTOR_NORMALIZATION_EPSILON) continue;

		toMate.x /= dist;
		toMate.y /= dist;

		// Alignment with the attack direction (dot with +/-X).
		const float alignment = toMate.x * attackDir.x + toMate.y * attackDir.y;
		if (alignment < Config::PASS_TARGET_ALIGNMENT_MIN) continue;

		const float nearestOpp = std::sqrt(nearestOpponentDistSq(world, ownerIsHome, matePos));
		const float spaceScore = std::clamp(nearestOpp / Config::AI_PASS_TARGET_SPACE_BONUS_DISTANCE, 0.f, 1.25f);

		// Score: alignment matters most, then space, then a small distance penalty.
		const float score = alignment * 1.2f + spaceScore * 1.0f - dist * 0.0015f;
		if (score <= bestScore) continue;

		bestScore = score;
		bestId = id;
	}

	if (bestId >= 0) return bestId;

	// Fallback: nearest teammate (even if behind).
	int nearestId = -1;
	float nearestDistSq = std::numeric_limits<float>::max();
	for (int id = teamStart; id <= teamEndInclusive; ++id) {
		if (id == ownerId) continue;
		if (id == goalkeeperId) continue; // SANTI 28/04/2026

		const sf::Vector2f matePos = world.players()[id].getPosition();
		const sf::Vector2f d = matePos - ownerPos;
		const float dsq = d.x * d.x + d.y * d.y;
		if (dsq >= nearestDistSq) continue;
		nearestDistSq = dsq;
		nearestId = id;
	}

	return nearestId;
}



// ============================================================================
// WORLD IMPLEMENTATION
// ============================================================================
// Step 4 deliverable: World now owns all game objects and can produce a
// raw GameStatePacket snapshot without any match logic (score, timer, rules).
// ============================================================================

// ----------------------------------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------------------------------
// SANTI: Setup pitch bounds and call deterministic kickoff initialization.
World::World()
{
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
// SANTI: Called by constructor and also used when a goal is scored (to restart).
void World::resetKickoff()
{

	// SANTI: define which goal is left/right once per reset.
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

	// SANTI 28/04/2026: Restart must cancel any in-flight guided travel state.
	// Without this, a pass/shot that was mid-flight could keep moving the ball
	// after a goal or kickoff reset, because Ball::update() would continue the old travel.
	mBall.cancelGuidedTravel();

	// SANTI 28/04/2026: Clear interaction cooldowns on restart.
	// Kickoff should always be immediately playable.
	mBall.setStealCooldown(0.f);

	// SANTI 28/04/2026: Clear per-player steal retry cooldowns.
	mStealRetryCooldownSec.fill(0.f);

	// SANTI 28/04/2026: Clear input edge history on restart.
	// Kickoff should not "inherit" held buttons from before a goal/state transition.
	mWasPassDown.fill(false);
	mWasShootDown.fill(false);

	// SANTI 28/04/2026: Deterministic kickoff ownership (MVP).
	// Home player 0 starts with the ball placed at midfield. This keeps "someone"
	// in possession right away and makes early gameplay less awkward than a loose
	// ball at center.
	//
	// If you later want alternating kickoffs or "team that conceded goal kicks off",
	// move this decision up into MatchState/KickoffState and pass the desired ownerId in.
	const int kickoffOwnerId = 0;

	// Place the kickoff owner so the ball ends up exactly on the center spot
	// when attached using the X offset.
	mPlayers[kickoffOwnerId].setPosition(sf::Vector2f(
		Config::FIELD_CENTER_X - Config::BALL_ATTACH_OFFSET_X,
		Config::FIELD_CENTER_Y
	));

	mBall.setOwner(kickoffOwnerId);
	attachBallToOwnerIfAny();
}

// ----------------------------------------------------------------------------
// SNAPSHOT PRODUCER (Step 4 deliverable)
// ----------------------------------------------------------------------------
// SANTI: Fill only the fields that World owns. Does NOT set score, timer, or
//        game state - those belong to Match.
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
}


void World::applyFrameMovement(const FrameInput& frameData, float dt) {
	if (dt <= 0.f) return;

	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {

		mPlayers[i].applyMoveDirection(frameData.inputs[i].moveDirection, dt);
	}
}

void World::attachBallToOwnerIfAny() {
	// SANTI: World-level possession mechanic.
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
	// SANTI: MVP interception/pickup rule.
	// This assigns ball ownership to the closest player inside pickupRadius.
	if (pickupRadius <= 0.f) return;

	if (mBall.getOwner() >= 0) return;

	// SANTI: Use Ball's cooldown as a "pickup grace timer" after kicks.
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
	// SANTI 28/04/2026: Simplified, deterministic steal/tackle resolution.
	// This is intentionally less complex than the old project's StealResolver:
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




void World::kickGuaranteedPassWithInterception(int ownerId) {
	// SANTI: A "guaranteed pass" is still initiated by input, but World owns the
	// mechanics (target choice + interception query + ball kick).
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;
	if (mBall.getOwner() != ownerId) return;

	// SANTI 28/04/2026: Don't start a new pass if one is already in flight.
	if (mBall.isGuidedInFlight()) return;

	const bool ownerIsHome = isHomePlayerId(ownerId);
	const float sign = ownerIsHome ? 1.f : -1.f;
	const sf::Vector2f ownerOffset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f startPos = mPlayers[ownerId].getPosition() + ownerOffset;

	const int receiverId = chooseBestPassReceiverId(*this, ownerId);
	if (receiverId < 0) return;

	// Pass to a fixed endpoint (target's current position) like the old project.
	// This avoids "homing passes" and keeps networking deterministic.
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
	// SANTI: A "guaranteed shot" aims at opponent goal center, but defenders can
	// intercept along a corridor before the ball reaches the goal.
	if (ownerId < 0) return;
	if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;
	if (mBall.getOwner() != ownerId) return;

	const bool ownerIsHome = isHomePlayerId(ownerId);

	// SANTI 28/04/2026: Don't start a new shot if one is already in flight.
	if (mBall.isGuidedInFlight()) return;

	const float sign = ownerIsHome ? 1.f : -1.f;
	const sf::Vector2f ownerOffset(Config::BALL_ATTACH_OFFSET_X * sign, 0.f);

	const sf::Vector2f startPos = mPlayers[ownerId].getPosition() + ownerOffset;

	// SANTI 28/04/2026: Lane selection (center vs near-post) based on defender spacing.
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
	const sf::Vector2f finalEnd = intercepted ? intercept.interceptPoint : intendedEnd;

	// If intercepted, defender gains possession at the end. If not, the shot ends loose.
	const int finalOwnerId = intercepted ? static_cast<int>(intercept.interceptorId) : -1;

	mBall.setPosition(startPos);
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));
	mBall.beginGuidedTravel(finalEnd, Config::GUARANTEED_SHOT_SPEED_MAX, finalOwnerId);
}

bool World::isPassPressed(const FrameInput& frameData, int playerId) const {
	// SANTI 28/04/2026: Edge detection for passDown (J key).
	// "Pressed" means down now, but was not down last tick.
	if (playerId < 0) return false;
	if (playerId >= static_cast<int>(Config::kNumPlayers)) return false;

	const std::size_t idx = static_cast<std::size_t>(playerId);
	const bool down = frameData.inputs[idx].passDown;
	return down && !mWasPassDown[idx];
}

bool World::isShootPressed(const FrameInput& frameData, int playerId) const {
	// SANTI 28/04/2026: Edge detection for shootDown (K key).
	if (playerId < 0) return false;
	if (playerId >= static_cast<int>(Config::kNumPlayers)) return false;

	const std::size_t idx = static_cast<std::size_t>(playerId);
	const bool down = frameData.inputs[idx].shootDown;
	return down && !mWasShootDown[idx];
}

void World::commitActionButtonHistory(const FrameInput& frameData) {
	// SANTI 28/04/2026: Commit DOWN-state history for edge detection.
	// Must be called once per simulation tick (during Playing) after you've read edges.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		mWasPassDown[i] = frameData.inputs[i].passDown;
		mWasShootDown[i] = frameData.inputs[i].shootDown;
	}
}


// ============================================================================
// SUMMARY OF SANTI CHANGES (Step 4)
// ============================================================================
// 1) World::World() constructor:
//      - Sets mPitchBounds using Config::WINDOW_WIDTH/HEIGHT.
//      - Calls resetKickoff() to initialize all objects.
// 2) World::resetKickoff():
//      - Initializes 8 players with correct teams and goalkeeper flags.
//      - Positions goalkeepers at fixed X (GOALKEEPER_X_LEFT/RIGHT) and center Y.
//      - Positions outfield players using formation constants.
//      - Resets ball to center, clears owner, zeroes velocity.
// 3) World::writeRawState(GameStatePacket& out) const:
//      - Fills only World-owned fields: pitchBounds, ball (pos/vel/owner),
//        and all players (position, velocity, facing, team, isGK, isLunging).
//      - Does NOT touch score, timer, currentState, or controlled player IDs.
// ============================================================================
