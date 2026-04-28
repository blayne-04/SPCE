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

void SettingsMenuState::handleInput(GameEngine& engine, sf::Event& event) {
	// TODO: Implement input handling for settings menu.
}

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

void PauseMenuState::update(GameEngine& engine, float dt) {
	// TODO: Implement update logic for pause menu.
}

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

void ClientPlayingState::update(GameEngine& engine, float dt) {
	constexpr std::uint8_t kFallbackAwayPlayerId = 4;

	const std::uint8_t playerId =
		mHaveState ? mLastState.controlledAwayPlayerId : kFallbackAwayPlayerId;

	InputPacket input{};

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

	mNetwork.sendPlayerInput(input);

	if (mNetwork.receiveLatestGameState(mLastState)) {
		mHaveState = true;
	}
}

void ClientPlayingState::render(sf::RenderWindow& window) {
	// Draw game world (DO NOT call window.clear()).
	sf::RectangleShape background(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	background.setFillColor(sf::Color::Blue);
	window.draw(background);
}

// ============================================================================
// HOST PLAYING STATE
// ============================================================================
// Host (authoritative) side of the networked match.
// Responsibilities:
//   - Simulate the game (not yet implemented in this stub).
//   - Poll incoming inputs from the client.
//   - Send authoritative snapshots back to the client.
//
// Important:
//   - The host learns the client's (ip, port) from the first received INPUT.
//   - sendGameState() does nothing until a client has been discovered.
// ============================================================================

HostPlayingState::HostPlayingState()
{
	mNetwork.startHost(Config::HOST_PORT);
	mFrame = 0;
}

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

	GameStatePacket state{};
	state.frameNumber = mFrame++;

	// Populate pitchBounds (required for client rendering).
	state.pitchBounds = sf::FloatRect(
		sf::Vector2f(0.f, 0.f),
		sf::Vector2f(
			static_cast<float>(Config::WINDOW_WIDTH),
			static_cast<float>(Config::WINDOW_HEIGHT)
		)
	);
	mNetwork.sendGameState(state); // does nothing until a client INPUT arrives.
}

void HostPlayingState::render(sf::RenderWindow& window) {
	// Draw game world (DO NOT call window.clear()).
	sf::RectangleShape background(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	background.setFillColor(sf::Color::Red);
	window.draw(background);
}

// ============================================================================
// SINGLEPLAYER PLAYING STATE
// ============================================================================
// Placeholder for local single-player mode (no networking).
// Currently only shows a green background and allows pause.
// ============================================================================

void SinglePlayerPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
		if (keyPressed->code == sf::Keyboard::Key::P) {
			engine.pushState(std::make_unique<PauseMenuState>());
		}
	}
}

void SinglePlayerPlayingState::update(GameEngine& engine, float dt) {
	// TODO: Implement single-player simulation.
}

void SinglePlayerPlayingState::render(sf::RenderWindow& window) {
	// Draw game world (DO NOT call window.clear()).
	sf::RectangleShape background(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
	background.setFillColor(sf::Color::Green);
	window.draw(background);
}
