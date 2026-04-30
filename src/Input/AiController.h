#pragma once

#include "../Common/Packets.h"
#include "../Common/Constants.h" // SANTI 29/04/26: Config::kNumPlayers
#include <array>                 // SANTI 28/04/2026: per-player AI timers

// SANTI 29/04/26: This header is intentionally named AIController.h to match the
// class name. This avoids confusion and avoids case-sensitive include failures
// when building on Linux or in CI.
class AIController {
public:
	AIController() = default;

	// Takes playerID + snapshot + dt, returns AI InputPacket (DOWN-state buttons).
	// SANTI 28/04/2026: dt is required so AI can use small action cooldown timers.
	InputPacket getAIInput(std::uint8_t playerID, const GameStatePacket& gameStatePacket, float dt);

private:
	// SANTI 28/04/2026: Per-player "ball action" cooldown (seconds).
	// Ensures pass/shoot are one-tick impulses, not held buttons.
	std::array<float, Config::kNumPlayers> mBallActionCooldownSec{};

	// SANTI 28/04/2026: Final-third stall timer (seconds) per player.
	// If an AI carrier stays in the attacking final third too long, we force
	// a pass/shot so the game does not stall in front of goal.
	std::array<float, Config::kNumPlayers> mFinalThirdStallSec{};
};

