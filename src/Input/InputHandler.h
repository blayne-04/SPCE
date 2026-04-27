#pragma once

#include "../Common/Packets.h"

// ============================================================================
// INPUT HANDLER
// ============================================================================
// Captures raw hardware input (keyboard) and converts it into
// an InputPacket. This is a pure generator – no networking code here.
// SANTI: Removed dependency on NetworkManager.h.
// ============================================================================

class InputHandler {
public:
	// SANTI: Changed signature – returns InputPacket for a specific player ID.
	// Reads current keyboard/mouse state and builds the DOWN-state fields
	// (shootDown, passDown, etc.) as required by the host-authoritative protocol.
	InputPacket getLocalInput(std::uint8_t playerID);
};