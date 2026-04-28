#include "GameEngine.h"
#include "../States/EngineState.h"

GameEngine::GameEngine() 
    : mWindow(sf::VideoMode({1280, 720}), "Super Copa Peru Evolution") {
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

        if (!mStates.empty()) {
            mStates.back()->tick(*this, dt);
        }

        mWindow.clear();
        for (auto& state : mStates) {
            state->render(mWindow);
        }
        mWindow.display();
    }
}
