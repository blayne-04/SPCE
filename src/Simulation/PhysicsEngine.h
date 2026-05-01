#pragma once

/**
 * @file PhysicsEngine.h
 * @brief Pure geometry and collision helpers for simulation code.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help draft/format documentation comments and
 * to sanity-check the public API shape for geometry-only helpers (player separation, segment corridor
 * interception). The team implemented and validated the final behavior through play-testing.
 *
 * Example prompt used:
 * "Review this PhysicsEngine header for a C++/SFML soccer game. Suggest concise
 * Doxygen comments for a geometry-only API (player separation + segment corridor
 * interception) that uses player ID ranges and does not depend on match rules."
 */

#include "Simulation/World.h"
#include <cstdint>

/**
 * @brief Result from a segment-corridor interception test.
 */
struct SegmentInterceptionResult {
	bool hasInterception = false;
	std::uint8_t interceptorId = 255;      // 255 = "none" sentinel
	sf::Vector2f interceptPoint{ 0.f, 0.f };
};

/**
 * @class PhysicsEngine
 * @brief Stateless helper class for physics queries that do not own game rules.
 */
class PhysicsEngine {
public:
	/**
	 * @brief Push overlapping players apart so they do not stack visually.
	 * @param world World containing all players.
	 * @param dt Delta time in seconds.
	 */
	static void resolvePlayerSeparation(World& world, float dt);

	/**
	 * @brief Find the first defender inside a pass/shot corridor.
	 *
	 * @param world Read-only world state.
	 * @param start Segment start point.
	 * @param end Segment end point.
	 * @param defendingIdStart First defender ID to consider.
	 * @param defendingIdEndInclusive Last defender ID to consider.
	 * @param corridorRadius Radius around the segment that counts as an interception.
	 * @return Earliest interception result, or hasInterception=false.
	 */
	static SegmentInterceptionResult findFirstSegmentInterception(
		const World& world,
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		std::uint8_t defendingIdStart,
		std::uint8_t defendingIdEndInclusive,
		float corridorRadius);
};
