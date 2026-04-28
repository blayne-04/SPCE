#pragma once

#include <SFML/Graphics.hpp>
#include "../Input/InputHandler.h"
#include "../Input/AIController.h"
#include "../Core/Renderer.h"
#include <stdint.h>
#include <stdio.h>

inline constexpr std::uint8_t HOST_ID = 0; // Reserve host player ID


/* Forward Declaration */
class GameEngine;

// ============================================================================
// BASE ENGINE STATE (ABSTRACT)
// ============================================================================
// All game states derive from this interface. Each state handles its own
// input, update logic, and rendering. The GameEngine manages a stack of states.
// ============================================================================

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

// ============================================================================
// START MENU STATE
// ============================================================================
// Initial menu. Handles H (host) and C (client) keys to start networking.
// ============================================================================

class StartMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
};
        Exit
    };

    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
        Join
    };

class SettingsMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

// ============================================================================
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
// Semi-transparent overlay. Typically pushed on top of a playing state.
// ============================================================================

class PauseMenuState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

// ============================================================================
// CLIENT PLAYING STATE
// ============================================================================
// Non-authoritative side of networked match.
// Responsibilities:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    InputHandler mInputHandler;
    std::uint8_t mMyPlayerID = 255;
    Renderer mRenderer;

class ClientPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
private: 
    InputHandler mInputHandler;
};

// ============================================================================
// HOST PLAYING STATE
// ============================================================================
// Authoritative side of networked match.
// Responsibilities:
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
//   - Send authoritative snapshots back to client.
    InputHandler mInputHandler;
    AIController mAiController;
    std::uint8_t mMyPlayerID = 1;
    Renderer mRenderer;

class HostPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
private:
    InputHandler mInputHandler;
};

// ============================================================================
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    InputHandler mInputHandler;
	AIController mAiController;
    std::uint8_t mMyPlayerID = 1;
    Renderer mRenderer;
};class SinglePlayerPlayingState : public EngineState {
public:
    void tick(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
private:
    InputHandler mInputHandler;
};