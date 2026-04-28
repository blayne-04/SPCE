#pragma once

#include "../Common/Packets.h"

class AIController {
public:
	/* Best to be explicit about default behavior */
	AIController() = default;

	/* Takes uint8_t playerID and gameStatePacket, returns AI Input packet */
	InputPacket getAIInput(std::uint8_t playerID, const GameStatePacket& gameStatePacket);
};