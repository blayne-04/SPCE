#pragma once
#include <SFML/Graphics.hpp>
#include "../Simulation/Match.h"
#include "../Network/NetworkManager.h"
#include <vector>
#include <memory>

class EngineState;

struct EngineContext {
    sf::RenderWindow& window;
    Match& match;
    NetworkManager& network;
    std::function<void(std::unique_ptr<EngineState>)> transitionTo;
};

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    void run();

    /* State transition stack helpers, i.e. when you pause, push to top of stack, pop when exiting pause and you're back in the game */
    void pushState(std::unique_ptr<EngineState> state);
    void popState();
    void transitionTo(std::unique_ptr<EngineState> state);

    sf::RenderWindow& getWindow() { return mWindow; }
    Match& getMatch() { return mMatch; }
    NetworkManager& getNetwork() { return mNetworkManager; }

private:
    sf::RenderWindow mWindow;
    std::vector<std::unique_ptr<EngineState>> mStates;
    Match mMatch;
    NetworkManager mNetworkManager;

    /* Handle OS Events e.g. (Maximize window, Minimize window, Close Window) */
    void processOsEvents();
};
