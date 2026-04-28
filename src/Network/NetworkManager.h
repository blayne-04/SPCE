#pragma once

#include <SFML/Network.hpp>
#include "../Common/Packets.h"
#include <queue>

using uint16 = unsigned short;

class NetworkManager {

public:

	NetworkManager() = default;

	void startHost(uint16 localPort);

	void startClient(const sf::IpAddress& remoteAddress, uint16 remotePort);

	void pollIncomingInputs(std::queue<InputPacket>& inputQueue);

	void sendGameState(const GameStatePacket& gameStatePacket);

	void sendPlayerInput(const InputPacket& inputPacket);

	bool receiveLatestGameState(GameStatePacket& outState);


private:

	sf::UdpSocket mSocket;

	uint16 mLocalPort;

	sf::IpAddress mRemoteAddress;

	uint16 mRemotePort;

};
