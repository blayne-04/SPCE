#pragma once

#include <SFML/Graphics.hpp>
#include "../Input/InputHandler.h"
#include "../Core/Renderer.h"
#include "../Input/AiController.h"
#include <stdint.h>
#include <stdio.h>

/* Forward declaration */
class GameEngine;

class EngineState {
public:
	EngineState() = default;
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
    StartMenuState();
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;
private:
    sf::Texture mBackgroundTex;
    std::optional<sf::Sprite> mBackgroundSprite;

    sf::Texture mButtonSheet;
    std::optional<sf::Sprite> mBtnPlay;
    std::optional<sf::Sprite> mBtnHost;
    std::optional<sf::Sprite> mBtnJoin;
    std::optional<sf::Sprite> mBtnSettings;

    /* Helper to see if a sprite is clicked */
    bool isSpriteClicked(sf::Sprite& sprite, sf::RenderWindow& window);
};

class SettingsMenuState : public EngineState {
public:
    SettingsMenuState();
    void tick(GameEngine& engine, float dt) override;
    void render(GameEngine& engine) override;

private:
    sf::Texture mBgTex;
    sf::Texture mPanelTex;
    sf::Texture mExitTex;
    sf::Texture mSettingWordsTex;
    sf::Texture mVolumeHighTex;
    sf::Texture mVolumeMidTex;
    sf::Texture mVolumeLowTex;

    sf::Texture mBrightnessHighTex;
    sf::Texture mBrightnessMidTex;
    sf::Texture mBrightnessLowTex;
    //sf::Texture mSettingWords;
    std::optional<sf::Sprite> mBg;
    std::optional<sf::Sprite> mPanel;
    std::optional<sf::Sprite> mExitBtn;
    std::optional<sf::Sprite> mSettingWords;

    std::optional<sf::Sprite> mVolumeIcon;
    std::optional<sf::Sprite> mBrightnessIcon;

    // 0 = LOW, 1 = MID, 2 = HIGH
    int mVolumeState = 2;
    int mBrightnessState = 1;
    bool mMouseWasDown = false;

    bool isMouseOver(const sf::FloatRect& bounds, sf::RenderWindow& window);
    void updateIcons();
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
    GameStatePacket mLatestState;
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