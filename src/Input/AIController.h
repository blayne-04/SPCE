#pragma once

/**
 * @file AIController.h
 * @brief Input generator for computer-controlled soccer players.
 *
 * AI assistance disclosure:
 * A generative AI assistant (DeepSeek) was used in a limited way to help draft/format documentation comments
 * and to suggest a simple "cooldown-based" structure so AI pass/shoot actions behave like one-tick
 * impulses (instead of being held every frame). The team tuned the heuristics through play-testing.
 *
 * Prompt used:
 * "Review this AIController header for a C++/SFML soccer game. Suggest clear Doxygen
 * comments and a simple API that generates InputPacket values from a GameStatePacket,
 * including per-player cooldowns to avoid holding pass/shoot every frame."
 */

#include "../Common/Packets.h"
#include "../Common/Constants.h"
#include <array>

 // This header name intentionally matches the class name. This avoids confusion and
 // helps prevent case-sensitive include issues on Linux/CI.
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
	// Per-player "ball action" cooldown (seconds). Ensures pass/shoot are one-tick
	// impulses, not held buttons.
	std::array<float, Config::kNumPlayers> mBallActionCooldownSec{};

	// Final-third stall timer (seconds) per player. If an AI carrier stays in the
	// attacking final third too long, force a pass/shot so play does not stall.
	std::array<float, Config::kNumPlayers> mFinalThirdStallSec{};
};

