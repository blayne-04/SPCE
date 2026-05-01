#pragma once

/**
 * @file NetworkManager.h
 * @brief UDP mailroom for host/client packet exchange.
 *
 * AI assistance disclosure:
 * A generative AI assistant (Codex) was used in a limited way to review non-blocking UDP receive
 * loops and to sanity-check the "type-tagged packet" approach (so INPUT/STATE packets are
 * not mis-parsed). The team designed and implemented the networking architecture and then
 * validated it via local host/client play-testing.
 *
 * Prompt used:
 * "Review this UDP NetworkManager interface. Suggest a safe packet-type tagging approach and
 * a non-blocking receive loop pattern that can keep only the latest STATE snapshot. Keep the
 * API small and do not add gameplay rules."
 */

#include <SFML/Network.hpp>
#include "../Common/Packets.h"
#include <queue>

using uint16 = unsigned short;

/**
 * @class NetworkManager
 * @brief Owns one UDP socket and moves packets between EngineState and the wire.
 *
 * This class intentionally does not know soccer rules. It only sends and
 * receives typed packets.
 */
class NetworkManager {
public:
	NetworkManager() = default;

	/**
	 * @brief Close the UDP socket and forget any remote endpoint.
	 * Used when a player exits a match back to the main menu. This prevents an
	 * old host/client socket from staying bound while the user is no longer in
	 * that match.
	 */
	void stop();

	/**
	 * @brief Bind the socket as host on a known port.
	 * @param localPort Port that clients will connect to.
	 */
	void startHost(uint16 localPort);

	/**
	 * @brief Bind the socket as client and store the host endpoint.
	 * @param hostAddress Host machine IP address.
	 * @param hostPort Host UDP port.
	 */
	void startClient(const sf::IpAddress& hostAddress, uint16 hostPort);

	/** @brief Client sends a repeated join request until assigned a player ID. */
	void sendJoinRequest();

	/**
	 * @brief Client polls for host assignment.
	 * @param outId Filled with assigned player ID when successful.
	 * @return true if an assignment was received.
	 */
	bool pollIdAssignment(std::uint8_t& outId);

	/** @brief Host drains valid client InputPackets into outQueue. */
	void pollIncomingInputs(std::queue<InputPacket>& outQueue);

	/** @brief Host sends one authoritative GameStatePacket to the known client. */
	void sendGameState(const GameStatePacket& gameStatePacket);

	/** @brief Client sends its current InputPacket to the host. */
	void sendPlayerInput(const InputPacket& inputPacket);

	/**
	 * @brief Return true once this socket knows a remote client endpoint.
	 *
	 * UDP has no persistent connection state. For the one-client MVP, the host
	 * considers the client "connected" after receiving any valid JOIN_REQUEST
	 * or INPUT packet and storing that sender's IP/port.
	 */
	bool hasRemoteClient() const;

	/**
	 * @brief Client drains state packets and keeps only the newest frame.
	 * @param outState Replaced by the newest received state.
	 * @return true if at least one valid state packet was received.
	 */
	bool receiveLatestGameState(GameStatePacket& outState);

private:
	sf::UdpSocket mSocket;
	uint16 mLocalPort = 0;
	sf::IpAddress mRemoteAddress = sf::IpAddress::Any;
	uint16 mRemotePort = 0;
};
