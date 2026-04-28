#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

// SANTI
#include "../Input/InputHandler.h"
#include "../Core/Renderer.h"
#include "../Common/Packets.h"
#include <queue>
#include "../Input/AiController.h"

class GameEngine;

// Base state API used by GameEngine.cpp
class EngineState {
public:
	virtual ~EngineState() = default;

	virtual void handleInput(GameEngine& engine, sf::Event& event) =
		0;
	virtual void tick(GameEngine& engine, float dt) = 0;
	virtual void render(GameEngine& engine) = 0;
};

class StartMenuState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
};

class SettingsMenuState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
};

class PauseMenuState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
};

class ClientPlayingState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;

private:
	bool mStarted = false;
	bool mHaveState = false;
	std::uint32_t mNextInputSequence = 0;

	InputHandler mInput{};
	Renderer mRenderer{};

	GameStatePacket mLastState{};
};

class HostPlayingState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;

private:
	bool mStarted = false;

	InputHandler mInput;
	AIController mAi;   // SANTI
	Renderer mRenderer;

	GameStatePacket mLastState;
};


class SinglePlayerPlayingState final : public EngineState {
public:
	void handleInput(GameEngine& engine, sf::Event& event) override;
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
};