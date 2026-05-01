#include "Ball.h"

/**
 * @file Ball.cpp
 * @brief Ball physics, ownership, and guided travel implementation.
 *
 * AI assistance disclosure:
 * A generative AI assistant (DeepSeek) was used in a limited way to help draft documentation comments
 * and suggest small helper extractions.
 * The team implemented the actual behavior and validated it via builds and play-testing.
 *
 * Prompt used:
 * "Review this Ball implementation for a C++/SFML soccer game. Suggest concise
 * Doxygen comments and guard-return patterns for guided travel vs velocity motion,
 * without changing runtime behavior."
 */

#include "../Common/Constants.h" // Config::...
#include <algorithm>             // std::clamp
#include <cmath>                 // std::sqrt

 // Helper for normalization (guard returns keep callers simple).
static sf::Vector2f normalizeOrZero(sf::Vector2f v) {
	const float lenSq = v.x * v.x + v.y * v.y;
	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	if (lenSq <= eps * eps) return sf::Vector2f(0.f, 0.f);

	const float invLen = 1.f / std::sqrt(lenSq);
	v.x *= invLen;
	v.y *= invLen;
	return v;
}

void Ball::applyKick(sf::Vector2f velocity) {
	// Any explicit kick cancels guided travel state.
	// Guided passes/shots own the ball position directly until they resolve.
	mGuided = GuidedTravelState{};

	clearOwner();
	mVelocity = velocity;

	// "Pickup grace" after any kick/pass/shot.
	// This prevents the ball from being immediately re-possessed on the same frame
	// it is kicked (since it starts very close to the owner).
	//
	// NOTE: The member name is still mStealCooldown (historical). For MVP, treat it
	// as a generic "cannot be picked up" timer. If you later implement steals with
	// a different rule, rename/split this variable then.
	mStealCooldown = Config::POST_KICK_PICKUP_DELAY_SECONDS;
}

void Ball::applyPass() {
	// "Guaranteed pass" MVP version with no target selection yet:
	// kick forward and bias toward center Y so it looks reasonable.
	const int owner = mOwnerID;
	if (owner < 0) return;

	const float sign = (owner < 4) ? 1.f : -1.f; // 0-3 home attacks right, 4-7 away attacks left
	const sf::Vector2f start = getPosition();
	const sf::Vector2f target(start.x + sign * 220.f, Config::FIELD_CENTER_Y);

	sf::Vector2f dir = normalizeOrZero(target - start);
	if (dir.x == 0.f && dir.y == 0.f) return;

	applyKick(dir * Config::GUARANTEED_PASS_SPEED);
}

void Ball::applyShot() {
	// "Guaranteed shot" MVP version: aim at opponent goal center.
	const int owner = mOwnerID;
	if (owner < 0) return;

	const bool homeHasBall = (owner < 4);

	const float goalX = homeHasBall
		? (Config::RIGHT_GOAL_X + Config::GOAL_WIDTH * 0.5f)
		: (Config::LEFT_GOAL_X + Config::GOAL_WIDTH * 0.5f);

	const sf::Vector2f start = getPosition();
	const sf::Vector2f target(goalX, Config::GOAL_CENTER_Y);

	sf::Vector2f dir = normalizeOrZero(target - start);
	if (dir.x == 0.f && dir.y == 0.f) return;

	// Pick one speed; you can tune later.
	applyKick(dir * Config::GUARANTEED_SHOT_SPEED_MAX);
}

void Ball::update(const float dt) {
	if (dt <= 0.f) return;

	// Cooldown ticks regardless of owner/flight state.
	// We use this as a generic "interaction disabled" timer:
	// - after a kick: prevents immediate pickup in the same frame
	// - during guided travel: prevents pickup/steal while ball is in flight
	// - after guided resolve to an owner: can be used as short possession protection
	if (mStealCooldown > 0.f) {
		mStealCooldown = std::max(0.f, mStealCooldown - dt);
	}

	// Guided travel update (guaranteed passes/shots).
	if (mGuided.active) {
		mGuided.elapsedSec += dt;

		const float duration = std::max(mGuided.durationSec, Config::VECTOR_NORMALIZATION_EPSILON);
		const float t = std::clamp(mGuided.elapsedSec / duration, 0.f, 1.f);

		const sf::Vector2f pos = mGuided.start + (mGuided.end - mGuided.start) * t;
		setPosition(pos);

		// Snapshot consistency: guided travel is not velocity-based.
		mVelocity = sf::Vector2f(0.f, 0.f);

		if (t < 1.f) return;

		// Resolve: assign possession only at the end (your rule: don't switch early).
		const int finalOwnerId = mGuided.finalOwnerId;
		mGuided = GuidedTravelState{};

		if (finalOwnerId >= 0) {
			setOwner(finalOwnerId);

			// Short protection window after receiving/intercepting
			// to avoid instant "tackle spam" on the same tick we resolve.
			mStealCooldown = std::max(mStealCooldown, Config::BALL_POSSESSION_PROTECTION_SECONDS);
			return;
		}

		// Shot that reaches its target stays loose.
		clearOwner();
		mStealCooldown = std::max(mStealCooldown, Config::POST_KICK_PICKUP_DELAY_SECONDS);
		return;
	}

	// If owned, the owning system should attach it (World). Do nothing here.
	if (mOwnerID >= 0) return;

	// Integrate.
	sf::Vector2f pos = getPosition() + (mVelocity * dt);

	// Keep inside pitch (simple bounce).
	const float r = Config::BALL_RADIUS;
	const float minX = r;
	const float maxX = static_cast<float>(Config::WINDOW_WIDTH) - r;
	const float minY = r;
	const float maxY = static_cast<float>(Config::WINDOW_HEIGHT) - r;

	if (pos.x < minX) { pos.x = minX; mVelocity.x = -mVelocity.x; }
	if (pos.x > maxX) { pos.x = maxX; mVelocity.x = -mVelocity.x; }
	if (pos.y < minY) { pos.y = minY; mVelocity.y = -mVelocity.y; }
	if (pos.y > maxY) { pos.y = maxY; mVelocity.y = -mVelocity.y; }

	setPosition(pos);

	// Friction (MVP).
	mVelocity *= Config::BALL_FRICTION;
}


void Ball::kickToward(const sf::Vector2f& target, float speed) {
	const sf::Vector2f start = getPosition();
	sf::Vector2f dir = target - start;

	const float lenSq = dir.x * dir.x + dir.y * dir.y;
	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	if (lenSq <= eps * eps) return;

	const float invLen = 1.f / std::sqrt(lenSq);
	dir.x *= invLen;
	dir.y *= invLen;

	applyKick(dir * speed); // applyKick takes a VELOCITY vector
}

void Ball::beginGuidedTravel(const sf::Vector2f& end, float travelSpeed, int finalOwnerId) {
	// Caller already decided 'end' and 'finalOwnerId' (receiver or interceptor).
	if (travelSpeed <= 0.f) return;

	const sf::Vector2f start = getPosition();
	const sf::Vector2f delta = end - start;
	const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

	// Guard: degenerate travel.
	if (distance <= Config::VECTOR_NORMALIZATION_EPSILON) return;

	mGuided.active = true;
	mGuided.start = start;
	mGuided.end = end;
	mGuided.durationSec = std::max(Config::GUARANTEED_PASS_MIN_DURATION, distance / travelSpeed);
	mGuided.elapsedSec = 0.f;
	mGuided.finalOwnerId = finalOwnerId;

	// Ball is loose during flight (possession is assigned only when travel ends).
	clearOwner();
	mVelocity = sf::Vector2f(0.f, 0.f);

	// Prevent pickup/steal while the guided travel is active.
	mStealCooldown = std::max(mStealCooldown, mGuided.durationSec);
}
