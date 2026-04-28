#include "MenuInputHandler.h"

void MenuInputHandler::handleEvent(const sf::Event& event, GameEngine& engine, StartMenuState& state) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        onKeyPressed(*key, engine, state);
    else if (const auto* mouse = event.getIf<sf::Event::MouseMoved>())
        onMouseMoved(*mouse, state);
    else if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>())
        onMouseClicked(*click, engine, state);
}

void MenuInputHandler::onKeyPressed(const sf::Event::KeyPressed& key, GameEngine& engine, StartMenuState& state) {
    // Arrow key navigation, Enter to select, Escape to close dropdown
}

void MenuInputHandler::onMouseMoved(const sf::Event::MouseMoved& mouse, StartMenuState& state) {
    // Update hover state — which button is the mouse over?
}

void MenuInputHandler::onMouseClicked(const sf::Event::MouseButtonPressed& mouse, GameEngine& engine, StartMenuState& state) {
    // Check click against button bounds, trigger selectOption or dropdown toggle
}

bool MenuInputHandler::isMouseOver(const sf::Vector2f& mousePos, const sf::Vector2f& buttonPos, const sf::Vector2f& buttonSize) {
    return mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonSize.x &&
        mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonSize.y;
}