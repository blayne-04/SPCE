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
	if (!mStates.empty()) {
		mStates.pop_back();
	}
}

void GameEngine::transitionTo(std::unique_ptr<EngineState> state) {
	if (!mStates.empty()) {
		mStates.pop_back();
	}
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
		float dt = clock.restart().asSeconds();

		processOsEvents();

        // Call tick() on the active state (top of stack)
        if (!mStates.empty()) {
            mStates.back()->tick(*this, dt);
        }

        // Render all states in the stack (for overlay support, e.g., pause menu over game)
        mWindow.clear();
        for (auto& state : mStates) {
            state->render(*this);
        }
        mWindow.display();
    }
}
