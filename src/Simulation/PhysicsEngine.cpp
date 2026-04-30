
#include "PhysicsEngine.h"
#include "../Common/Constants.h"   // Config::PLAYER_MIN_X etc
#include <algorithm>               // std::clamp
#include <cmath>                   // std::sqrt

namespace physicsEngine {
	// SANTI: Clamp to pitch bounds so separation pushes cannot shove players out of play.
	static sf::Vector2f clampPlayerPos(sf::Vector2f pos) {
		pos.x = std::clamp(pos.x, Config::PLAYER_MIN_X, Config::PLAYER_MAX_X);
		pos.y = std::clamp(pos.y, Config::PLAYER_MIN_Y, Config::PLAYER_MAX_Y);
		return pos;
	}

	// SANTI: Dot product helper for segment projection.
	static float dot(sf::Vector2f a, sf::Vector2f b) {
		return a.x * b.x + a.y * b.y;
	}


	// SANTI: Updates "best so far" interception if candidate blocks earlier along segment.
	static void evaluateInterceptionCandidate(
		const sf::Vector2f& candidatePos,
		std::uint8_t candidateId,
		const sf::Vector2f& start,
		const sf::Vector2f& segment,
		float segmentLenSq,
		float corridorRadius,
		float& inOutBestT,
		SegmentInterceptionResult& inOutResult)
	{
		// Guard: degenerate segment handled by caller.
		if (segmentLenSq <= 0.f) return;

		const sf::Vector2f toCandidate = candidatePos - start;
		const float t = dot(toCandidate, segment) / segmentLenSq;

		// Guard: only count interceptions strictly inside the segment, and only if earlier than current best.
		if (t <= 0.f) return;
		if (t >= 1.f) return;
		if (t >= inOutBestT) return;

		const sf::Vector2f projected = start + segment * t;
		const sf::Vector2f delta = candidatePos - projected;
		const float distSq = delta.x * delta.x + delta.y * delta.y;

		// Guard: outside the travel corridor.
		if (distSq > corridorRadius * corridorRadius) return;

		inOutBestT = t;
		inOutResult.hasInterception = true;
		inOutResult.interceptorId = candidateId;
		inOutResult.interceptPoint = projected;
	}



} // physicsEngine




void PhysicsEngine::resolvePlayerSeparation(World& world, float dt) {
	if (dt <= 0.f) return;

	auto& players = world.players();

	const float minDist = Config::PLAYER_MIN_SEPARATION;
	const float minDistSq = minDist * minDist;

	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	const float epsSq = eps * eps;

	// O(n^2) pairwise separation is fine for 8 players.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		for (std::size_t j = i + 1; j < Config::kNumPlayers; ++j) {
			const sf::Vector2f pi = players[i].getPosition();
			const sf::Vector2f pj = players[j].getPosition();

			sf::Vector2f delta = pj - pi;
			const float distSq = delta.x * delta.x + delta.y * delta.y;

			// Guard: not overlapping.
			if (distSq >= minDistSq) continue;

			// Guard: too close to normalize safely.
			if (distSq <= epsSq) continue;

			const float dist = std::sqrt(distSq);

			// Normalize delta to get push direction.
			delta.x /= dist;
			delta.y /= dist;

			const float overlap = minDist - dist;

			// SANTI: Push both players away from each other equally.
			// The push constant is tuned in Config::PLAYER_SEPARATION_PUSH.
			const sf::Vector2f push =
				delta * (overlap * Config::PLAYER_SEPARATION_PUSH * dt * 0.5f);

			players[i].setPosition(physicsEngine::clampPlayerPos(pi - push));
			players[j].setPosition(physicsEngine::clampPlayerPos(pj + push));
		}
	}
}

SegmentInterceptionResult PhysicsEngine::findFirstSegmentInterception(
	const World& world,
	const sf::Vector2f& start,
	const sf::Vector2f& end,
	std::uint8_t defendingIdStart,
	std::uint8_t defendingIdEndInclusive,
	float corridorRadius)
{
	SegmentInterceptionResult result{};
	result.hasInterception = false;
	result.interceptorId = 255;
	result.interceptPoint = end;

	if (corridorRadius <= 0.f) return result;

	const sf::Vector2f segment = end - start;
	const float segmentLenSq = segment.x * segment.x + segment.y * segment.y;

	// Guard: segment too small to project onto.
	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	if (segmentLenSq <= eps * eps) return result;

	// Guard: invalid range.
	if (defendingIdStart > defendingIdEndInclusive) return result;

	float bestT = 1.f;

	// SANTI: Iterate defending player IDs only (no Team class).
	const auto& players = world.players();
	for (std::uint8_t id = defendingIdStart; id <= defendingIdEndInclusive; ++id) {
		const sf::Vector2f candidatePos = players[id].getPosition();

		physicsEngine::evaluateInterceptionCandidate(
			candidatePos,
			id,
			start,
			segment,
			segmentLenSq,
			corridorRadius,
			bestT,
			result);
	}

	return result;
}
