#include "EngineState.h"
#include "../Core/GameEngine.h"
#include "../Common/Constants.h"
#include <iostream>

/**************************
* START MENU STATE
**************************/

StartMenuState::StartMenuState() {
	// 1. Background Setup (using the homeMenu art)
	if (mBackgroundTex.loadFromFile("assets/backgrounds/homeMenu.png")) {
		mBackgroundSprite.emplace(mBackgroundTex);
		mBackgroundSprite->setScale({
			static_cast<float>(Config::WINDOW_WIDTH) / mBackgroundTex.getSize().x,
			static_cast<float>(Config::WINDOW_HEIGHT) / mBackgroundTex.getSize().y
			});
	}

	// 2. Refactored Standardized Button Logic
	if (mButtonSheet.loadFromFile("assets/textures/menuButtonsSheet.png")) {
		const int cellW = 200; // Width of one button cell
		const int cellH = 90;  // Height of one button cell
		const int gap = 2;     // Transparent gap between buttons on sheet

		// Slicing logic: (Index * (Width + Gap))
		auto getRect = [&](int index) {
			return sf::IntRect({ index * (cellW + gap), 0 }, { cellW, cellH });
			};

		// Construct using SFML 3.1's mandatory-texture constructor
		mBtnPlay.emplace(mButtonSheet, getRect(0));     // Index 0
		mBtnHost.emplace(mButtonSheet, getRect(1));     // Index 1
		mBtnJoin.emplace(mButtonSheet, getRect(2));     // Index 2
		mBtnSettings.emplace(mButtonSheet, getRect(3)); // Index 3

		// 3. Vertical Stack Layout (Centered between player sprites)
		float centerX = Config::WINDOW_WIDTH / 2.0f;
		float startY = 300.0f;   // Moves the whole group up/down
		float vSpacing = 80.0f;  // Fixed gap between button centers

		auto setupBtn = [&](std::optional<sf::Sprite>& opt, float yPos) {
			if (opt) {
				opt->setOrigin({ cellW / 2.0f, cellH / 2.0f });
				opt->setScale({ 0.8f, 0.8f }); // Adjust if they are too big for the gap
				opt->setPosition({ centerX, yPos });
			}
			};

		// Applying the vertical order: Play -> Join -> Host -> Settings
		setupBtn(mBtnPlay, startY);
		setupBtn(mBtnJoin, startY + vSpacing);
		setupBtn(mBtnHost, startY + vSpacing * 2);
		setupBtn(mBtnSettings, startY + vSpacing * 3);
	}
}

void StartMenuState::tick(GameEngine& engine, float dt) {
	auto& window = engine.getWindow();

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
		// Must check if optional has value before dereferencing with *
		if (mBtnPlay && isSpriteClicked(*mBtnPlay, window)) {
			engine.transitionTo(std::make_unique<SinglePlayerPlayingState>());
		}
		else if (mBtnHost && isSpriteClicked(*mBtnHost, window)) {
			/* Host initialization */
			engine.getNetwork().startHost(Config::HOST_PORT);
			std::optional<sf::IpAddress> myIP = sf::IpAddress::getLocalAddress();
			if (myIP) {
				std::cout << "Host: Listening on " << myIP->toString() 
				          << ":" << Config::HOST_PORT << std::endl;
				std::cout << "Players should connect to this IP address!" << std::endl;
			}
			engine.transitionTo(std::make_unique<HostPlayingState>());
		}
		else if (mBtnJoin && isSpriteClicked(*mBtnJoin, window)) {
			/* Client connects to host */
			std::optional<sf::IpAddress> hostIP = sf::IpAddress::resolve("10.57.227.196");
			if (hostIP) {
				engine.getNetwork().startClient(*hostIP, Config::HOST_PORT);
				std::cout << "Client: Connecting to " << hostIP->toString() 
				          << ":" << Config::HOST_PORT << std::endl;
			} else {
				std::cout << "Error: Could not resolve host!" << std::endl;
			}
			engine.transitionTo(std::make_unique<ClientPlayingState>());
		}
		else if (mBtnSettings && isSpriteClicked(*mBtnSettings, window)) {
			engine.transitionTo(std::make_unique<SettingsMenuState>());
		}
	}
}

void StartMenuState::render(GameEngine& engine) {
	auto& window = engine.getWindow();

	// Draw in order: Background first, then buttons
	if (mBackgroundSprite) window.draw(*mBackgroundSprite);
	if (mBtnPlay) window.draw(*mBtnPlay);
	if (mBtnHost) window.draw(*mBtnHost);
	if (mBtnJoin) window.draw(*mBtnJoin);
	if (mBtnSettings) window.draw(*mBtnSettings);
}

bool StartMenuState::isSpriteClicked(sf::Sprite& sprite, sf::RenderWindow& window) {
	sf::Vector2i mousePos = sf::Mouse::getPosition(window);
	// mapPixelToCoords handles scaling/view differences automatically
	sf::Vector2f translatedPos = window.mapPixelToCoords(mousePos);
	return sprite.getGlobalBounds().contains(translatedPos);
}

/**************************
* SETTINGS MENU STATE
**************************/

void SettingsMenuState::tick(GameEngine& engine, float dt) 
{
	/* TODO: Implement update logic for settings menu */
}

void SettingsMenuState::render(GameEngine& engine)
{
	/* TODO: Implement rendering for settings menu */
}

/**************************
* PAUSE MENU STATE
**************************/

void PauseMenuState::tick(GameEngine& engine, float dt) 
{
	/* TODO: Implement input handling for pause menu */
}

void PauseMenuState::render(GameEngine& engine) 
{
	auto& window = engine.getWindow();
	
	/* Draw semi-transparent overlay (DO NOT call window.clear()) */
	sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(Config::WINDOW_WIDTH), static_cast<float>(Config::WINDOW_HEIGHT)));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));  /* Black with 180 alpha */
	window.draw(overlay);
	/* TODO: Draw pause menu options */
}

/**************************
* CLIENT PLAYING STATE
**************************/

void ClientPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	/* Handshake phase : request player ID if we don't have one yet */
	if (mMyPlayerID == 255) {
		network.sendJoinRequest();

		if (!network.pollIdAssignment(mMyPlayerID)) {
			return; /* Still waiting for assignment */
		}

		std::cout << "Assigned Player ID: " << static_cast<int>(mMyPlayerID) << std::endl;
	}

	/* Normal gameplay : send input to host */
	InputPacket clientInput = mInputHandler.getLocalInput(mMyPlayerID);
	network.sendPlayerInput(clientInput);

	/* Receive and apply latest game state from host */
	GameStatePacket latestGameState;
	if (network.receiveLatestGameState(latestGameState)) {
		mLatestState = latestGameState; /* Store for rendering */
	}
}

void ClientPlayingState::render(GameEngine& engine) 
{
	auto& window = engine.getWindow();
	mRenderer.render(window, mLatestState);
}

/**************************
* HOST PLAYING STATE
**************************/

void HostPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();
	auto& match = engine.getMatch();

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}
	
	/* Handle any new clients trying to join */
	network.handleHandshakeRequests();

	/* Intake host input in slot 0 */
	FrameInput frameData;
	frameData.inputs[0] = mInputHandler.getLocalInput(0); /* HOST_ID = 0 */

	/* Track human player slots */
	bool isHuman[Config::kNumPlayers] = { false };
	isHuman[0] = true;

	/* Collect client packets */
	std::queue<InputPacket> remotePackets;
	network.pollIncomingInputs(remotePackets);

	/* Process incoming packets */
	while (!remotePackets.empty()) {
		InputPacket p = remotePackets.front();
		remotePackets.pop();

		if (p.playerId >= Config::kNumPlayers) { 
			std::cout << "Received packet with invalid player ID: " << static_cast<int>(p.playerId) << std::endl;
			continue;
		}

		frameData.inputs[p.playerId] = p;
		isHuman[p.playerId] = true;
	}

	/* Get current game state for AI */
	GameStatePacket currentState;
	match.getGameState(currentState);

	/* Fill gaps with packets from AI */
	for (uint8_t i = 0; i < Config::kNumPlayers; ++i) {
		if (!isHuman[i]) {
			/* Pass player ID and game state packet to AI */
			frameData.inputs[i] = mAiController.getAIInput(i, currentState);
		}
	}

	/* Update the match */
	match.update(frameData, dt);

	/* Get updated game state and send to clients */
	match.getGameState(currentState);
	network.sendGameState(currentState);
}

void HostPlayingState::render(GameEngine& engine) 
{
	auto& window = engine.getWindow();
	auto& match = engine.getMatch();
	
	/* Get current game state and render */
	GameStatePacket currentState;
	match.getGameState(currentState);
	mRenderer.render(window, currentState);
}

/**************************
* SINGLEPLAYER STATE
**************************/

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& match = engine.getMatch();

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	/* Build frame input */
	FrameInput frameData;
	frameData.inputs[0] = mInputHandler.getLocalInput(0); /* Player is always slot 0 */

	/* Get current game state for AI */
	GameStatePacket currentState;
	match.getGameState(currentState);

	/* Fill remaining slots with AI */
	for(uint8_t i = 1; i < Config::kNumPlayers; ++i) {
		frameData.inputs[i] = mAiController.getAIInput(i, currentState);
	}

	/* Update match */
	match.update(frameData, dt);
}

void SinglePlayerPlayingState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();
	auto& match = engine.getMatch();
	
	/* Get current game state and render */
	GameStatePacket currentState;
	match.getGameState(currentState);
	mRenderer.render(window, currentState);
}