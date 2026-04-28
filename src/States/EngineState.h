#pragma once

#include <SFML/Graphics.hpp>

/* Forward Declaration */
class GameEngine;

/**
 * Base class for all engine states.
 * States handle high-level game flow (menus, gameplay modes, etc.)
 */
class EngineState {
public:
    virtual ~EngineState() = default;

    /**
     * Called once per frame. Handle input and update state logic.
     * @param engine - Reference to GameEngine for state transitions
     * @param dt - Delta time in seconds since last frame
     */
    virtual void tick(GameEngine& engine, float dt) = 0;

    /**
     * Called once per frame after tick. Draw to the window.
     * @param window - SFML render target
     */
    virtual void render(sf::RenderWindow& window) = 0;
};

class StartMenuState : public EngineState {
public:
    StartMenuState();

    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    enum class MenuOption {
        Singleplayer,
        Multiplayer,
        Settings,
        Exit
    };

    enum class MultiplayerOption {
        Host,
        Join
    };

    MenuOption mSelectedOption = MenuOption::Singleplayer;
    bool mMultiplayerDropdownOpen = false;
    MultiplayerOption mSelectedMultiplayerOption = MultiplayerOption::Host;

    // Helper methods
    void handleKeyboardInput(GameEngine& engine, const sf::Event::KeyPressed& keyPress);
    void handleMouseInput(GameEngine& engine, const sf::Event::MouseButtonPressed& mousePress);
    void selectOption(GameEngine& engine);
    void drawButton(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size,
        const std::string& text, bool isSelected, const sf::Color& color = sf::Color(80, 80, 120));
};

class SettingsMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class PauseMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class ClientPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class HostPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class SinglePlayerPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};