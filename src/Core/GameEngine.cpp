/**
 * @file GameEngine.cpp
 * @brief Main SFML window loop and state-stack implementation.
 */

#include "GameEngine.h"
#include "../States/EngineState.h"
#include "../Common/Constants.h"
#include <optional>

GameEngine::GameEngine()
	: mWindow(sf::VideoMode({ Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT }), "Super Copa Peru Evolution") {
	mWindow.setFramerateLimit(60);
	pushState(std::make_unique<StartMenuState>());
}

GameEngine::~GameEngine() = default;

void GameEngine::pushState(std::unique_ptr<EngineState> state) {
	mStates.push_back(std::move(state));
}

void GameEngine::popState() {
	if (mStates.empty()) return;

	mStates.pop_back();
}

void GameEngine::transitionTo(std::unique_ptr<EngineState> state) {
	if (!mStates.empty()) mStates.pop_back();

	mStates.push_back(std::move(state));
}

void GameEngine::processOsEvents() {
	while (const std::optional<sf::Event> event = mWindow.pollEvent()) {
		if (event->is<sf::Event::Closed>()) {
			mWindow.close();
		}
	}
}

void GameEngine::run() {
	sf::Clock clock;

	while (mWindow.isOpen() && !mStates.empty()) {
		const float dt = clock.restart().asSeconds();

		processOsEvents();

		// The active state is the top of the stack.
		if (!mStates.empty()) mStates.back()->tick(*this, dt);

		// Render every state so overlay states, such as pause, can draw on top.
		mWindow.clear();

		for (auto& state : mStates) {
			state->render(*this);
		}

		mWindow.display();
	}
}
