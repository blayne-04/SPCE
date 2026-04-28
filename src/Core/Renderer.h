#pragma once

#include <SFML/Graphics.hpp>
#include "Common/Packets.h"

// ============================================================================
// RENDERER
// ============================================================================
// Consumes authoritative GameStatePacket snapshots and draws the game world.
// No direct dependency on Match/World/Simulation - only on the network contract.
// ============================================================================

class Renderer {
public:
	// SANTI: Default constructor removed (compiler-generated is sufficient).

	// SANTI: Changed render() to consume GameStatePacket instead of Match.
	// Pass by const reference to avoid copying the entire snapshot.
	void render(sf::RenderWindow& window, const GameStatePacket& gameState);

	// Draw scoreboard, timer, and game state text.
	// Parameters are extracted from GameStatePacket for convenience.
	void renderHUD(sf::RenderWindow& window, int homeScore, int awayScore, float matchTimerSec, int stateID);
};
