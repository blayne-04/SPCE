#include "Ball.h"

#include "../Common/Constants.h" // Config::...
#include <algorithm>             // std::clamp
#include <cmath>                 // std::sqrt

// SANTI: Helper for normalization (never-nest style: guard returns).
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
	// SANTI: Generic "ball is now in flight" action.
	clearOwner();
	mVelocity = velocity;

	// SANTI: "Pickup grace" after any kick/pass/shot.
	// This prevents the ball from being immediately re-possessed on the same frame
	// it is kicked (since it starts very close to the owner).
	//
	// NOTE: The member name is still mStealCooldown (historical). For MVP, treat it
	// as a generic "cannot be picked up" timer. If you later implement steals with
	// a different rule, rename/split this variable then.
	mStealCooldown = Config::POST_KICK_PICKUP_DELAY_SECONDS;
}

void Ball::applyPass() {
	// SANTI: "Guaranteed pass" MVP version with no target selection yet:
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
	// SANTI: "Guaranteed shot" MVP version: aim at opponent goal center.
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

	// SANTI: If owned, the owning system should attach it (PlayingState). Do nothing here.
	if (mOwnerID >= 0) return;

	// Cooldown tick.
	if (mStealCooldown > 0.f) {
		mStealCooldown = std::max(0.f, mStealCooldown - dt);
	}

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
