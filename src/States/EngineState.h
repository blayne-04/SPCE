#pragma once

#include <SFML/Graphics.hpp>
#include "../Input/InputHandler.h"
#include "../Input/AIController.h"
#include "../Core/Renderer.h"
#include <stdint.h>
#include <stdio.h>

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
     * @param engine - Reference to GameEngine for state transitions and resources
     * @param dt - Delta time in seconds since last frame
     */
    virtual void tick(GameEngine& engine, float dt) = 0;

    /**
     * Called once per frame after tick. Draw to the window.
     * @param engine - Reference to GameEngine for accessing window and resources
     */
    virtual void render(GameEngine& engine) = 0;
};





class StartMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
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
    // Getters — MenuInputHandler needs to read and modify these
    MenuOption getSelectedOption() const { return mSelectedOption; }
    MultiplayerOption getSelectedMultiplayerOption() const { return mSelectedMultiplayerOption; }
    bool isMultiplayerDropdownOpen() const { return mMultiplayerDropdownOpen; }

    // Setters — MenuInputHandler calls these to update state
    void setSelectedOption(MenuOption option) { mSelectedOption = option; }
    void setSelectedMultiplayerOption(MultiplayerOption option) { mSelectedMultiplayerOption = option; }
    void setMultiplayerDropdownOpen(bool open) { mMultiplayerDropdownOpen = open; }

   
private:
   

    MenuOption mSelectedOption = MenuOption::Singleplayer;
    bool mMultiplayerDropdownOpen = false;
    MultiplayerOption mSelectedMultiplayerOption = MultiplayerOption::Host;

    // Helper methods
   // void handleKeyboardInput(GameEngine& engine, const sf::Event::KeyPressed& keyPress);// part of menu input handler class
    //void handleMouseInput(GameEngine& engine, const sf::Event::MouseButtonPressed& mousePress); // part of menu input handler class
    //
    void selectOption(GameEngine& engine);
    // SFML resources
    sf::Font mFont;
    sf::Texture mBackgroundTexture;
    sf::Sprite mBackground;
    sf::Vector2f mWindowSize;
    MenuInputHandler mInputHandler;

    void drawButton(sf::RenderWindow& window, const sf::Vector2f& position, const sf::Vector2f& size,
        const std::string& text, bool isSelected, const sf::Color& color = sf::Color(80, 80, 120));
    friend class MenuInputHandler;
};

class SettingsMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
};

class PauseMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
};

class ClientPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    InputHandler mInputHandler;
    std::uint8_t mMyPlayerID = 255;
    Renderer mRenderer;
};

class HostPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    InputHandler mInputHandler;
    AIController mAiController;
    std::uint8_t mMyPlayerID = 0;
    Renderer mRenderer;
};

class SinglePlayerPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    InputHandler mInputHandler;
    AIController mAiController;
    std::uint8_t mMyPlayerID = 0;
    Renderer mRenderer;
};