#pragma once

#include <SFML/Network.hpp>
#include "../Common/Packets.h"
#include <queue>

using uint16 = unsigned short;

class NetworkManager {
public:
	NetworkManager() = default;

	// Host mode: bind to a fixed port
	void startHost(uint16 localPort);

	// Client mode: connect to host
	void startClient(const sf::IpAddress& hostAddress, uint16 hostPort);

	// Handshake methods
	void sendJoinRequest();
	bool pollIdAssignment(std::uint8_t& outId);
	void handleHandshakeRequests();

	// Gameplay methods
	void pollIncomingInputs(std::queue<InputPacket>& outQueue);
	void sendGameState(const GameStatePacket& gameStatePacket);
	void sendPlayerInput(const InputPacket& inputPacket);
	bool receiveLatestGameState(GameStatePacket& outState);

private:
	sf::UdpSocket mSocket;
	uint16 mLocalPort = 0;
	sf::IpAddress mRemoteAddress = sf::IpAddress::Any;
	uint16 mRemotePort = 0;
};
