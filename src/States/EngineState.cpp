#include "EngineState.h"
#include "../Core/GameEngine.h"
#include "../Common/Constants.h"

// ============================================================================
// START MENU STATE
// ============================================================================
// Entry point of the game. Allows the player to choose between:
//   - H key: start as host (HostPlayingState)
//   - C key: start as client (ClientPlayingState)
// ============================================================================

void StartMenuState::handleInput(GameEngine& engine, sf::Event& event) {
	// TODO: Implement full input handling for start menu.

	// SANTI - TEST - DELETE AFTER
	const auto* key = event.getIf<sf::Event::KeyPressed>();
	if (!key) return;

	if (key->code == sf::Keyboard::Key::H) {
		engine.transitionTo(std::make_unique<HostPlayingState>());
		return;
	}
	if (key->code == sf::Keyboard::Key::C) {
		engine.transitionTo(std::make_unique<ClientPlayingState>());
		return;
	}
}

void StartMenuState::update(GameEngine& engine, float dt) {
	// TODO: Implement update logic for start menu (animations, etc.).
}

void StartMenuState::render(sf::RenderWindow& window) {
	// Draw menu background (DO NOT call window.clear() here).
	sf::RectangleShape background(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	background.setFillColor(sf::Color(50, 50, 100));  // Dark blue.
	window.draw(background);
	// TODO: Draw menu elements (text, buttons, etc.).
}

// ============================================================================
// SETTINGS MENU STATE
// ============================================================================
// Placeholder for settings screen (graphics, audio, controls).
// ============================================================================

void StartMenuState::render(GameEngine& engine) 
{

void SettingsMenuState::update(GameEngine& engine, float dt) {
	// TODO: Implement update logic for settings menu.
}

void SettingsMenuState::render(sf::RenderWindow& window) {
	// Draw settings background (DO NOT call window.clear()).
	sf::RectangleShape background(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	background.setFillColor(sf::Color(50, 100, 50));  // Dark green.
	window.draw(background);
	// TODO: Draw settings UI elements.
}

// ============================================================================
// PAUSE MENU STATE
// ============================================================================
// Semi-transparent overlay that appears when the player presses P.
// Typically used to resume the game or return to the main menu.
// ============================================================================

void PauseMenuState::handleInput(GameEngine& engine, sf::Event& event) {
	// TODO: Implement input handling for pause menu.
}

void SettingsMenuState::render(GameEngine& engine) 
{

void PauseMenuState::render(sf::RenderWindow& window) {
	// Draw semi-transparent overlay (DO NOT call window.clear()).
	sf::RectangleShape overlay(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));  // Black with 180 alpha.
	window.draw(overlay);
	// TODO: Draw pause menu options.
}

// ============================================================================
// CLIENT PLAYING STATE
// ============================================================================
// Client (non-authoritative) side of the networked match.
// Responsibilities:
//   - Send local inputs to the host (destination).
//   - Receive authoritative snapshots from the host.
//   - Render the latest snapshot.
//
// Network handshake:
//   - Initially no snapshot -> use fallback player ID (4).
//   - Send neutral inputs until first snapshot arrives.
//   - After snapshot received, use controlledAwayPlayerId from host.
// ============================================================================

ClientPlayingState::ClientPlayingState()
{
	mNetwork.startClient(sf::IpAddress::LocalHost, Config::HOST_PORT);

	// Before first STATE arrives, we have no controlledAwayPlayerId yet.
	mHaveState = false;
	mLastState = GameStatePacket{};
	mNextInputSequence = 0;
}

void ClientPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		if (keyPressed->code == sf::Keyboard::Key::P) {
			engine.pushState(std::make_unique<PauseMenuState>());
		}
	}
}

void PauseMenuState::render(GameEngine& engine)
{

	if (!mHaveState) {
		// Handshake: send a neutral INPUT so the host learns our (ip, port).
		input.playerId = playerId;
		input.moveDirection = sf::Vector2f(0.f, 0.f);
		input.shootDown = false;
		input.passDown = false;
		input.tackleDown = false;
		input.switchDown = false;
		input.lungeDown = false;
	}
	else {
		// Normal: send real keyboard DOWN-state.
		input = mInput.getLocalInput(playerId);
	}

	// Always stamp routing + sequencing last so it's consistent.
	input.playerId = playerId;
	input.inputSequence = mNextInputSequence++;

void ClientPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}
}

	if (mMyPlayerID == 255) {
		network.sendJoinRequest();

		if (!network.pollIdAssignment(mMyPlayerID)) {
			return;
		}

		std::cout << "Assigned Player ID: " << static_cast<int>(mMyPlayerID) << std::endl;
	}

	InputPacket clientInput = mInputHandler.pollInput(mMyPlayerID);
	network.sendPlayerInput(clientInput);

	GameStatePacket latestGameState;
	if (network.receiveLatestGameState(latestGameState)) {
		engine.getMatch().overwriteWorld(latestGameState);
	}
}

void ClientPlayingState::render(GameEngine& engine) 
{
	mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
void HostPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		if (keyPressed->code == sf::Keyboard::Key::P) {
			engine.pushState(std::make_unique<PauseMenuState>());
		}
	}
}

void HostPlayingState::update(GameEngine& engine, float dt) {
	std::queue<InputPacket> incoming;
	mNetwork.pollIncomingInputs(incoming); // learns client ip:port on first INPUT.

void HostPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();
	auto& match = engine.getMatch();

    /* Intake host input in slot 0 */
	FrameInput frameData;
	frameData.inputs[0] = mInputHandler.pollInput(HOST_ID);

    /* Track human player slots */
    bool isHuman[10] = { false };
    isHuman[0] = true;
    
    /* Collect client packets */
    std::queue<InputPacket> remotePackets;
    network.pollIncomingInputs(remotePackets);

    /* While there's incoming packets, process them */
    while (!remotePackets.empty()) {
        InputPacket p = remotePackets.front();
        remotePackets.pop();

		if (p.playerId >= 10) { 
            std::cout << "Received packet with invalid player ID: " << static_cast<int>(p.playerId) << std::endl;
            continue;
        }

        frameData.inputs[p.playerId] = p;
        isHuman[p.playerId] = true;
    }

    /* Fill gaps with packets from AI */
    for (uint8_t i = 0; i < 10; ++i) {
        if (!isHuman[i]) {
            /* Pass Id and match to each AI input call */
            frameData.inputs[i] = mAiController.getAIInput(i, match.getWorld());
        }
    }

    /* Call the update for match */
    match.update(frameData, dt);

    /* Send update across the network to all clients */
    network.sendGameState(match.getStatePacket());
}

void HostPlayingState::render(GameEngine& engine) 
{
	mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
}

/* ========================================= */
/*             SINGLEPLAYER                  */
/* ========================================= */

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt) 
{
    	auto& match = engine.getMatch();

		FrameInput frameData;
        frameData.inputs[0] = mInputHandler.pollInput(HOST_ID);

        for(uint8_t i = 1; i < 10; ++i) {
            frameData.inputs[i] = mAiController.getAIInput(i, match.getWorld());
		}

        match.update(frameData, dt);
}

void SinglePlayerPlayingState::render(GameEngine& engine)
{
    mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
}