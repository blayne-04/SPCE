#pragma once

#include "../Core/GameEngine.h"
#include <SFML/Graphics.hpp>

/* Forward Declaration */
class GameEngine;

class EngineState {
public:
    virtual ~EngineState() = default;

    virtual void handleInput(GameEngine& engine, sf::Event& event) = 0;
    virtual void update(GameEngine& engine, float dt) = 0;
    virtual void render(sf::RenderWindow& window) = 0;

    virtual void onEnter() {}
    virtual void onExit() {}
};