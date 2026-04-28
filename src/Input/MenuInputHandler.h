#pragma once
#include <SFML/Graphics.hpp>

// Forward declarations
class GameEngine;
class StartMenuState;

class MenuInputHandler {
public:
    // Called from StartMenuState::tick() — processes one event at a time
    void handleEvent(const sf::Event& event, GameEngine& engine, StartMenuState& state);

private:
    // Broken out by input type, mirrors what you already have in EngineState.h
    void onKeyPressed(const sf::Event::KeyPressed& key, GameEngine& engine, StartMenuState& state);
    void onMouseMoved(const sf::Event::MouseMoved& mouse, StartMenuState& state);
    void onMouseClicked(const sf::Event::MouseButtonPressed& mouse, GameEngine& engine, StartMenuState& state);

    // Utility — checks if a mouse position is inside a button's bounds
    bool isMouseOver(const sf::Vector2f& mousePos, const sf::Vector2f& buttonPos, const sf::Vector2f& buttonSize);
};