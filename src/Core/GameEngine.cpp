#include "GameEngine.h"

void GameEngine::pushState(std::unique_ptr<EngineState> state) {
    mStates.push_back(std::move(state));
    mStates.back()->onEnter();
}

void GameEngine::popState() {
    if (!mStates.empty()) {
        mStates.back()->onExit();
        mStates.pop_back();
    }
}

void GameEngine::transitionTo(std::unique_ptr<EngineState> state)
{
    popState();
    pushState(std::move(state));
}

void GameEngine::processOsEvents() {

}

void GameEngine::run() {
    mWindow.create(sf::VideoMode(Confi::WINDOW_SIZE), "Super Copa Peru Evolution");
    mWindow.setFramerateLimit(60);

    /* Source of truth for clock */
    sf::Clock clock;

    while (mWindow.isOpen()) {
        sf::Time deltaTime = clock.restart();
        
        /* Process OS Events */
        processOsEvents();

        /* Check if state is initialized, it should be startMenu */
        if (mStates.empty()) break;
           
        /* The state on top of the stack receives a deltaTime, and a ptr to the GameEngine for state mutation */
        mStates.back()->update(*this, deltaTime.asSeconds());

        /* Clear window and render the entire stack Credit: Gemini, Prompt: How do I render an entire stack of states in C++ */
        mWindow.clear();
        
        for (auto& state : mStates) {
            state->render(mWindow);
        }

        mWindow.display();
    }
}