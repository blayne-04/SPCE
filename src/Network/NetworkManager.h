#pragma once

/**
 * @file NetworkManager.h
 * @brief UDP mailroom for host/client packet exchange.
 *
 * AI disclosure:
 * The message-tagged UDP handshake, input polling, and latest-state snapshot
 * receive flow were implemented and documented with help from OpenAI Codex
 * because robust socket handling is beyond the normal CPTS 122 core topics.
 *
 * Prompt used:
 * "Help me implement a minimal SFML UDP NetworkManager for a host-authoritative
 * game. Use one socket, message type tags, a single-client handshake, queued
 * InputPacket polling, and latest GameStatePacket receive logic."
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

	/** @brief Host drains JOIN_REQUEST messages and replies with ASSIGNMENT. */
	void handleHandshakeRequests();

	/** @brief Host drains valid client InputPackets into outQueue. */
	void pollIncomingInputs(std::queue<InputPacket>& outQueue);

	/** @brief Host sends one authoritative GameStatePacket to the known client. */
	void sendGameState(const GameStatePacket& gameStatePacket);

	/** @brief Client sends its current InputPacket to the host. */
	void sendPlayerInput(const InputPacket& inputPacket);

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
