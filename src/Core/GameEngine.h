#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

/* Forward Declaration */
class EngineState;

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

private:
    sf::RenderWindow mWindow;
    std::vector<std::unique_ptr<EngineState>> mStates;

    /* Handle OS Events e.g. (Maximize window, Minimize window, Close Window) */
    void processOsEvents();
};