#pragma once

#include "../Common/Packets.h"
#include <array> // SANTI 28/04/2026: per-player AI timers

class AIController {
public:
	/* Best to be explicit about default behavior */
	AIController() = default;

	/* Takes playerID + snapshot + dt, returns AI InputPacket (DOWN-state buttons). */
	// SANTI 28/04/2026: dt is required so AI can use small action cooldown timers.
	// Without this, AI tends to hold passDown/shootDown forever, and because World
	// computes edges from DOWN state, the action triggers once and then the AI
	// freezes with moveDirection = (0,0).
	InputPacket getAIInput(std::uint8_t playerID, const GameStatePacket& gameStatePacket, float dt);

private:
	// SANTI 28/04/2026: Per-player "ball action" cooldown (seconds).
	// This is a tiny port of the old project's AIAttackDecisionSystem timers.
	// It ensures pass/shoot are one-tick impulses, not held buttons.
	std::array<float, Config::kNumPlayers> mBallActionCooldownSec{};
};
