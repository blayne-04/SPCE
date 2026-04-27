#pragma once

// SANTI: Removed all includes except Packets.h (no Match.h, no NetworkManager.h).
#include "../Common/Packets.h"

// ============================================================================
// AI CONTROLLER
// ============================================================================
// Generates InputPacket for computer-controlled players.
// Reads the same authoritative GameStatePacket that the renderer and network use,
// ensuring deterministic behavior across host and clients.
// ============================================================================

class AIController {
public:
	// SANTI: Removed default constructor (compiler-generated is sufficient).

	// SANTI: Changed signature - takes uint8_t playerID and const reference to
	// GameStatePacket. The AI uses the snapshot to decide movement and actions.
	InputPacket getAIInput(std::uint8_t playerID, const GameStatePacket& gameStatePacket);
};


/*
  SANTI Justification: keeps AI deterministic off the same snapshot the renderer/network uses.
  - pollIncomingInputs(std::queue<InputPacket>) must become
		pollIncomingInputs(std::queue<InputPacket>& outQueue).
  - Every UDP datagram starts with uint8 msgType (NetMsg::INPUT or NetMsg::STATE) before
	the payload, and you branch on it when receiving.
*/

