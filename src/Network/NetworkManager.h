#pragma once

#include <SFML/Network.hpp>
#include "../Common/Packets.h"

#include <queue>

using uint16 = unsigned short;

// ============================================================================
// NETWORK MANAGER
// ============================================================================
// The "mailroom" of the architecture.
//
// Single-client MVP rules:
// - Host binds to a fixed port and waits for INPUT packets.
// - Client binds to AnyPort (ephemeral local port) and sends INPUT to host.
// - Host learns the client endpoint (IP, port) from the first received INPUT.
// - Host sends STATE snapshots to that learned endpoint.
//
// Packet format:
// - Every UDP datagram starts with a 1-byte tag: NetMsg::{INPUT, STATE}
// - Then the payload struct (InputPacket or GameStatePacket), serialized via
//   the sf::Packet operators in Common/Packets.h.
// ============================================================================
//
// SANTI: This header was rewritten to match the current NetworkManager.cpp
// implementation (single-client, endpoint learned from first INPUT). The older
// "join request / id assignment / broadcast" API was removed because it was
// not implemented and was breaking compilation.

class NetworkManager {
public:
	NetworkManager() = default;

	// Host mode: bind to a fixed port; remote endpoint unknown until first INPUT arrives.
	void startHost(uint16 localPort);

	// Client mode: store host endpoint; bind to AnyPort so we can receive snapshots.
	void startClient(const sf::IpAddress& hostAddress, uint16 hostPort);

	// Host: drain the socket and push any valid INPUT packets into outQueue.
	void pollIncomingInputs(std::queue<InputPacket>& outQueue);

	// Host -> Client: send authoritative snapshot (no-op until host has learned client endpoint).
	void sendGameState(const GameStatePacket& gameStatePacket);

	// Client -> Host: send local input each tick.
	void sendPlayerInput(const InputPacket& inputPacket);

	// Client: drain the socket and keep the newest STATE snapshot (by frameNumber).
	bool receiveLatestGameState(GameStatePacket& outState);

private:
	sf::UdpSocket mSocket;

	// Local port is mainly for debugging/logging.
	uint16 mLocalPort = 0;

	// Remote endpoint:
	// - host mode: client endpoint learned from first INPUT
	// - client mode: host endpoint passed to startClient
	sf::IpAddress mRemoteAddress = sf::IpAddress::Any;
	uint16 mRemotePort = 0;
};
