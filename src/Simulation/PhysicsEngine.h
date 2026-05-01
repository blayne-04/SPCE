#pragma once

/**
 * @file PhysicsEngine.h
 * @brief Pure geometry and collision helpers for simulation code.
 *
 * AI disclosure:
 * The segment-corridor interception query and player-separation utility were
 * implemented and documented with help from OpenAI Codex because this geometry
 * is beyond basic CPTS 122 class/inheritance requirements.
 *
 * Prompt used:
 * "Help me port pass/shot interception logic into a new SFML soccer simulation.
 * Use player IDs instead of Team classes, keep PhysicsEngine geometry-only, and
 * return the first defender inside a segment corridor."
 */

#include "Simulation/World.h"
#include <cstdint> // SANTI: uint8_t

 // SANTI: Pure-geometry result for "segment corridor" interceptions.
 // This is used by pass/shot logic to decide if a defender should intercept.
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
