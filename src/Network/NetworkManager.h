#pragma once

#include <SFML/Network.hpp>
#include "../Common/Packets.h"
#include <queue>

using uint16 = unsigned short;

class NetworkManager {
public:

	// SANTI: Default constructor is sufficient (socket starts unbound).
	NetworkManager() = default;

	// Put socket into host mode: bind to a fixed port, wait for client inputs.
	void startHost(uint16 localPort);

	// Put socket into client mode: store host endpoint, bind to ephemeral port.
	void startClient(const sf::IpAddress& remoteAddress, uint16 remotePort);


	// SANTI: output-by-reference so caller actually receives inputs.
	// Drains UDP receive buffer and extracts all valid InputPackets.
	// Call this every host tick before building FrameInput.
	void pollIncomingInputs(std::queue<InputPacket>& outQueue);


	// Host -> Client: send authoritative snapshot.
	void sendGameState(const GameStatePacket& gameStatePacket);

	// Client -> Host: send local input.
	void sendPlayerInput(const InputPacket& inputPacket);

	// Client side: drain buffer, keep only the newest snapshot (by frameNumber).
	// Returns true if at least one valid GameStatePacket was received.
	bool receiveLatestGameState(GameStatePacket& outState);


private:

	sf::UdpSocket mSocket;       // Non-blocking UDP socket.
	uint16 mLocalPort = 0;       // Port this socket is bound to (0 = unbound).
	sf::IpAddress mRemoteAddress = sf::IpAddress::Any; // Destination endpoint (host or client). SANTI: placeholder until first endpoint known
	uint16 mRemotePort = 0;       // Destination port

};
