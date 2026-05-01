#pragma once

/**
 * @file GameEngine.h
 * @brief Main application loop and high-level shared services.
 */

#include <SFML/Graphics.hpp>
#include "../Simulation/Match.h"
#include "../Network/NetworkManager.h"
#include <vector>
#include <memory>

class EngineState;

/**
 * @class GameEngine
 * @brief Owns the window, state stack, Match, and NetworkManager.
 *
 * GameEngine does not contain soccer rules. It delegates to EngineState and
 * Match so the architecture stays separated.
 */
class GameEngine {
public:
    /** @brief Create the render window and initial game state. */
    GameEngine();

    /** @brief Destroy engine resources. */
    ~GameEngine();

    /** @brief Run the main event/update/render loop. */
    void run();

    /** @brief Push a state on top of the current state stack. */
    void pushState(std::unique_ptr<EngineState> state);

    /** @brief Pop the current top state. */
    void popState();

    /** @brief Replace the current state stack with one new state. */
    void transitionTo(std::unique_ptr<EngineState> state);

    /** @brief Access the shared SFML window. */
    sf::RenderWindow& getWindow() { return mWindow; }

    /** @brief Access the shared match/referee object. */
    Match& getMatch() { return mMatch; }

    /** @brief Access the shared network manager. */
    NetworkManager& getNetwork() { return mNetworkManager; }

private:
    sf::RenderWindow mWindow;
    std::vector<std::unique_ptr<EngineState>> mStates;
    Match mMatch;
    NetworkManager mNetworkManager;

    /* Handle OS Events e.g. (Close Window) */
    void processOsEvents();
};
