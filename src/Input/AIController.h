#pragma once

/**
 * @file AIController.h
 * @brief Input generator for computer-controlled soccer players.
 *
 * AI disclosure:
 * The tactical AI helpers and cooldown-based decision flow were written and
 * documented with help from OpenAI Codex because this is beyond typical CPTS
 * 122 data-structures logic.
 *
 * Prompt used:
 * "Port simple soccer AI into my host-authoritative SFML game. Generate
 * InputPacket values from a GameStatePacket, avoid networking dependencies,
 * add cooldowns for pass/shoot decisions, and keep the code readable."
 */

#include "../Common/Packets.h"
#include "../Common/Constants.h" // SANTI 29/04/26: Config::kNumPlayers
#include <array>                 // SANTI 28/04/2026: per-player AI timers

// SANTI 29/04/26: This header is intentionally named AIController.h to match the
// class name. This avoids confusion and avoids case-sensitive include failures
// when building on Linux or in CI.
/**
 * @class AIController
 * @brief Converts a snapshot into AI-controlled InputPacket decisions.
 *
 * The AI is intentionally a generator. It does not mutate World, Match, or
 * NetworkManager directly.
 */
class AIController {
public:
	AIController() = default;

	/**
	 * @brief Build one AI input packet for one player.
	 * @param playerID Player slot to control.
	 * @param gameStatePacket Current authoritative snapshot.
	 * @param dt Delta time in seconds.
	 * @return DOWN-state input for the requested player.
	 */
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

