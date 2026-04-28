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
    void render(GameEngine& engine) override;
private:
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