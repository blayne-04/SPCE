#include "AiController.h"
#include "../Common/Constants.h"
#include <cmath>

InputPacket AIController::getAIInput(std::uint8_t playerID, const GameStatePacket& s) {
	InputPacket out{};
	out.playerId = playerID;

	if (playerID >= Config::kNumPlayers) return out;

	const PlayerState& me = s.players[playerID];

	sf::Vector2f d = s.ballPosition - me.position;
	const float lenSq = d.x * d.x + d.y * d.y;

	const float eps = Config::VECTOR_NORMALIZATION_EPSILON;
	if (lenSq <= eps * eps) return out;

	const float invLen = 1.f / std::sqrt(lenSq);
	d.x *= invLen;
	d.y *= invLen;

	if (me.isGoalkeeper) {
		out.moveDirection = sf::Vector2f(0.f, (d.y >= 0.f) ? 1.f : -1.f);
		return out;
	}

	out.moveDirection = d;
	return out;
}