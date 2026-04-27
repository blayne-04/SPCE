#pragma once

#include <SFML/Graphics.hpp>

// SANTI: Necessary for class member attributes. Real OOP state, not globals.
#include "../Network/NetworkManager.h"
#include "../Input/InputHandler.h"
#include "../Common/Packets.h"


/* Forward Declaration */
class GameEngine;

class EngineState {
public:
	virtual ~EngineState() = default;

	virtual void handleInput(GameEngine& engine, sf::Event& event) = 0;
	virtual void update(GameEngine& engine, float dt) = 0;
	virtual void render(sf::RenderWindow& window) = 0;
};

class StartMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

class SettingsMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

class PauseMenuState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};

class ClientPlayingState : public EngineState {
public:

	// SANTI: Constructor
	ClientPlayingState();

	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;


private:
	// SANTI
	// Networking + input generators
	NetworkManager mNetwork;
	InputHandler mInput;
	// SANTI
	// Snapshot tracking
	GameStatePacket mLastState{};
	bool mHaveState = false;
	// SANTI
	// Client->host sequencing (monotonic, wraps naturally)
	std::uint32_t mNextInputSequence = 0;

	/*
	SANTI
	This keeps the handshake logic inside the client play state.
	 NetworkManager stays the "mailroom" and doesn't need to know game rules.
	*/


};

class HostPlayingState : public EngineState {
public:

	// SANTI
	HostPlayingState();
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;


private:
	// SANTI
	NetworkManager mNetwork;
	std::uint32_t mFrame = 0;
};


class SinglePlayerPlayingState : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void update(GameEngine& engine, float dt) override;
	void render(sf::RenderWindow& window) override;
};
