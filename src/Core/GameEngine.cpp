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
	if (mIsTickingState) {
		mPendingStateAction = PendingStateAction::Push;
		mPendingState = std::move(state);
		return;
	}

	mStates.push_back(std::move(state));
}

void GameEngine::popState() {
	if (mIsTickingState) {
		mPendingStateAction = PendingStateAction::Pop;
		mPendingState.reset();
		return;
	}

	if (mStates.empty()) return;

	mStates.pop_back();
}

void GameEngine::transitionTo(std::unique_ptr<EngineState> state) {
	if (mIsTickingState) {
		mPendingStateAction = PendingStateAction::Transition;
		mPendingState = std::move(state);
		return;
	}

	if (!mStates.empty()) mStates.pop_back();

	mStates.push_back(std::move(state));
}

void GameEngine::resetStateStack(std::unique_ptr<EngineState> state) {
	// Used when leaving an active match. A pause state sits on top of the match
	// state, so replacing only the top state would leave the old match alive
	// underneath the menu.
	if (mIsTickingState) {
		mPendingStateAction = PendingStateAction::ResetStack;
		mPendingState = std::move(state);
		return;
	}

	mStates.clear();
	mStates.push_back(std::move(state));
}

void GameEngine::resetMatch() {
	// Keep match lifetime centralized in GameEngine, but allow states to request
	// a clean match when the user starts/restarts/exits a game mode.
	mMatch.reset();
}

void GameEngine::resetNetwork() {
	// NetworkManager is also owned by GameEngine, so a mode exit must explicitly
	// close the old socket before returning to menus or starting another mode.
	mNetworkManager.stop();
}

void GameEngine::applyPendingStateAction() {
	if (mPendingStateAction == PendingStateAction::None) return;

	if (mPendingStateAction == PendingStateAction::Push) {
		mStates.push_back(std::move(mPendingState));
		mPendingStateAction = PendingStateAction::None;
		return;
	}

	if (mPendingStateAction == PendingStateAction::Pop) {
		if (!mStates.empty()) mStates.pop_back();
		mPendingStateAction = PendingStateAction::None;
		return;
	}

	if (mPendingStateAction == PendingStateAction::Transition) {
		if (!mStates.empty()) mStates.pop_back();
		mStates.push_back(std::move(mPendingState));
		mPendingStateAction = PendingStateAction::None;
		return;
	}

	if (mPendingStateAction == PendingStateAction::ResetStack) {
		mStates.clear();
		mStates.push_back(std::move(mPendingState));
		mPendingStateAction = PendingStateAction::None;
		return;
	}
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
		if (!mStates.empty()) {
			// State changes requested during tick() are deferred until tick()
			// returns. This avoids deleting/replacing the active state while one
			// of its member functions is still executing.
			mIsTickingState = true;
			mStates.back()->tick(*this, dt);
			mIsTickingState = false;
			applyPendingStateAction();
		}

		// Render every state so overlay states, such as pause, can draw on top.
		mWindow.clear();

		for (auto& state : mStates) {
			state->render(*this);
		}

		mWindow.display();
	}
}
