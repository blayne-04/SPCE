#pragma once

#include "Simulation/World.h"
#include <cstdint> // SANTI: uint8_t

// SANTI: Pure-geometry result for "segment corridor" interceptions.
// This is used by pass/shot logic to decide if a defender should intercept.
struct SegmentInterceptionResult {
	bool hasInterception = false;
	std::uint8_t interceptorId = 255;      // 255 = "none" sentinel
	sf::Vector2f interceptPoint{ 0.f, 0.f };
};

class PhysicsEngine {
public:
	static void resolvePlayerSeparation(World& world, float dt);

	// SANTI: Finds the earliest defender (smallest t along the segment) who is within
	// corridorRadius of the segment from start->end.
	//
	// defendingIdStart/EndInclusive define which IDs are allowed to intercept.
	// Example: if Home attacks, defending ids are 4..7.
	static SegmentInterceptionResult findFirstSegmentInterception(
		const World& world,
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		std::uint8_t defendingIdStart,
		std::uint8_t defendingIdEndInclusive,
		float corridorRadius);
};
