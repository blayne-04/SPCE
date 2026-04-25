#pragma once

#include "SFML/Graphics.hpp"

#include <queue>

#include "Match.h"
#include "Renderer.h"
#include "NetworkManager.h"


enum class OperationMode {
	SINGLE_PLAYER,
	HOST,
	CLIENT
};


class GameEngine {

public:

	GameEngine(OperationMode initMode);

	// runs the program, should be called after the constructor
	void run();

private:

	// class attributes

	sf::RenderWindow mWindow;

	OperationMode mOperationMode;

	Match mMatch;
	Renderer mRenderer;
	NetworkManager mNetworkManager;
	InputHandler mInputHandler;
	AIController mAIController;

	std::queue<InputPacket> clientQueues;


	// private functions for the different loops

	void runHostLoop();
	void runClientLoop();
	bool processWindoeEvents();


};
