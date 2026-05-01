#pragma once

/**
 * @file InputHandler.h
 * @brief Converts keyboard state into InputPacket values.
 */

#include "../Common/Packets.h"

// ============================================================================
// INPUT HANDLER
// ============================================================================
// Captures raw hardware input (keyboard) and converts it into
// an InputPacket. This is a pure generator - no networking code here.
// This module intentionally has no dependency on networking code.
// ============================================================================

/**
 * @class InputHandler
 * @brief Pure input generator with no networking or simulation ownership.
 */
class InputHandler {
public:
	/**
	 * @brief Read local keyboard state for one player.
	 * @param playerID Player ID that should be stamped into the packet.
	 * @return DOWN-state input packet for the current frame.
	 */
	InputPacket getLocalInput(std::uint8_t playerID);
};
