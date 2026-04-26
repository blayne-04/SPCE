#include "EngineState.h"
#include "../Core/GameEngine.h"

/* ========================================= */
/*               START MENU                  */
/* ========================================= */

void StartMenuState::handleInput(GameEngine& engine, sf::Event& event) {
    // TODO: Implement input handling for start menu
}

void StartMenuState::update(GameEngine& engine, float dt) {
    // TODO: Implement update logic for start menu
}

void StartMenuState::render(sf::RenderWindow& window) {
    // Draw menu - DO NOT call window.clear()
    sf::RectangleShape background(sf::Vector2f(1280, 720));
    background.setFillColor(sf::Color(50, 50, 100)); // Dark blue
    window.draw(background);
    // TODO: Draw menu elements
}

/* ========================================= */
/*           SETTINGS MENU                   */
/* ========================================= */

void SettingsMenuState::handleInput(GameEngine& engine, sf::Event& event) {
    // TODO: Implement input handling for settings menu
}

void SettingsMenuState::update(GameEngine& engine, float dt) {
    // TODO: Implement update logic for settings menu
}

void SettingsMenuState::render(sf::RenderWindow& window) {
    // Draw settings - DO NOT call window.clear()
    sf::RectangleShape background(sf::Vector2f(1280, 720));
    background.setFillColor(sf::Color(50, 100, 50)); // Dark green
    window.draw(background);
    // TODO: Draw settings UI
}

/* ========================================= */
/*            PAUSE MENU                     */
/* ========================================= */

void PauseMenuState::handleInput(GameEngine& engine, sf::Event& event) {
    // TODO: Implement input handling for pause menu
}

void PauseMenuState::update(GameEngine& engine, float dt) {
    // TODO: Implement update logic for pause menu
}

void PauseMenuState::render(sf::RenderWindow& window) {
    // Draw semi-transparent overlay - DO NOT call window.clear()
    sf::RectangleShape overlay(sf::Vector2f(1280, 720));
    overlay.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
    window.draw(overlay);
    // TODO: Draw pause menu options
}

/* ========================================= */
/*            CLIENT PLAY                    */
/* ========================================= */
void ClientPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::P) {
            engine.pushState(std::make_unique<PauseMenuState>());
        }
    }
}

void ClientPlayingState::update(GameEngine& engine, float dt) {}

void ClientPlayingState::render(sf::RenderWindow& window) {
    // Draw game - DO NOT call window.clear()
    sf::RectangleShape background(sf::Vector2f(1280, 720));
    background.setFillColor(sf::Color::Blue);
    window.draw(background);
}

/* ========================================= */
/*                HOST PLAY                  */
/* ========================================= */
void HostPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::P) {
            engine.pushState(std::make_unique<PauseMenuState>());
        }
    }
}

void HostPlayingState::update(GameEngine& engine, float dt) {}

void HostPlayingState::render(sf::RenderWindow& window) {
    // Draw game - DO NOT call window.clear()
    sf::RectangleShape background(sf::Vector2f(1280, 720));
    background.setFillColor(sf::Color::Red);
    window.draw(background);
}

/* ========================================= */
/*             SINGLEPLAYER                  */
/* ========================================= */
void SinglePlayerPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::P) {
            engine.pushState(std::make_unique<PauseMenuState>());
        }
    }
}

void SinglePlayerPlayingState::update(GameEngine& engine, float dt) {}

void SinglePlayerPlayingState::render(sf::RenderWindow& window) {
    // Draw game - DO NOT call window.clear()
    sf::RectangleShape background(sf::Vector2f(1280, 720));
    background.setFillColor(sf::Color::Green);
    window.draw(background);
}