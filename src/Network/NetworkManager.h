#pragma once

#include "SFML/Network.hpp"

using uint16 = unsigned short;


struct GameStatePacket {
	uint32_t sequenceNumber;
	float ballPosX;
	float ballPosY;
	float ballVelX;
	float ballVelY;
	int ballOwnerSlot;
	float playerPos[16];
	unsigned int homeScore;
	unsigned int awayScore;
	float matchTime;
	bool isOverTime;
	int stateID;
};



struct InputPacket {
	uint8_t slotID;
	sf::Vector2f moveDirection;
	bool passPressed;
	bool shootPressed;
	bool tackePressed;
	bool switchPlayerPressed;
};




class NetworkManager {

public:

	NetworkManager() = default;

	void startHost(uint16 localPort);

	void startClient(const sf::IpAddress& remoteAddress, uint16 remotePort);

	void pollIncomingInputs(std::queue<InputPacket>);

	void sendGameState(const GameStatePacket& gameStatePacket);

	void sendPlayerInput(const InputPacket& inputPacket);

	bool receiveLatestGameState(GameStatePacket& outState);


private:

	sf::UdpSocket mSocket;

	uint16 mLocalPort;

	sf::IpAddress mRemoteAddress;

	uint16 mRemotePort;



};
