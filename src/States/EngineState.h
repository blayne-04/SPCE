#pragma once

#include <SFML/Graphics.hpp>
#include "../Network/NetworkManager.h"   // SANTI: Necessary for class member attributes.
#include "../Input/InputHandler.h"       // Real OOP state, not globals.
#include "../Common/Packets.h"

// ============================================================================
// FORWARD DECLARATION
// ============================================================================

class GameEngine;

// ============================================================================
// BASE ENGINE STATE (ABSTRACT)
// ============================================================================
// All game states derive from this interface. Each state handles its own
// input, update logic, and rendering. The GameEngine manages a stack of states.
// ============================================================================

class EngineState {
public:
	virtual ~EngineState() = default;

	virtual void handleInput(GameEngine& engine, sf::Event& event) = 0;
	virtual void update(GameEngine& engine, float dt) = 0;
	virtual void render(sf::RenderWindow& window) = 0;
};

// ============================================================================
// START MENU STATE
// ============================================================================
// Initial menu. Handles H (host) and C (client) keys to start networking.
// ============================================================================

class StartMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

// ============================================================================
// SETTINGS MENU STATE
// ============================================================================
// Placeholder for graphics/audio/controls configuration.
// ============================================================================

class SettingsMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

// ============================================================================
// PAUSE MENU STATE
// ============================================================================
// Semi-transparent overlay. Typically pushed on top of a playing state.
// ============================================================================

class PauseMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

// ============================================================================
// CLIENT PLAYING STATE
// ============================================================================
// Non-authoritative side of networked match.
// Responsibilities:
//   - Send local inputs to host.
//   - Receive and render authoritative snapshots.
//   - Handshake: uses fallback player ID until first snapshot arrives.
// ============================================================================

class ClientPlayingState : public EngineState {
public:
	// SANTI: Constructor
	ClientPlayingState();

	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;

private:
	// SANTI: Networking + input generators
	NetworkManager mNetwork;
	InputHandler mInput;

	// SANTI: Snapshot tracking
	GameStatePacket mLastState{};
	bool mHaveState = false;

	// SANTI: Client->host sequencing (monotonic, wraps naturally)
	std::uint32_t mNextInputSequence = 0;

	/*
	 * SANTI: This keeps the handshake logic inside the client play state.
	 * NetworkManager stays the "mailroom" and doesn't need to know game rules.
	 */
};

// ============================================================================
// HOST PLAYING STATE
// ============================================================================
// Authoritative side of networked match.
// Responsibilities:
//   - Poll incoming inputs from client.
//   - Run game simulation (not yet implemented).
//   - Send authoritative snapshots back to client.
// ============================================================================

class HostPlayingState : public EngineState {
public:
	// SANTI: Constructor
	HostPlayingState();

	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;

private:
	// SANTI: Network manager and frame counter
	NetworkManager mNetwork;
	std::uint32_t mFrame = 0;
};

// ============================================================================
// SINGLEPLAYER PLAYING STATE
// ============================================================================
// Local offline mode (no networking). Placeholder implementation.
// ============================================================================

class SinglePlayerPlayingState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};
