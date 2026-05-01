#pragma once

/**
 * @file EngineState.h
 * @brief Application state classes for menu, gameplay, host, client, and single-player modes.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited, targeted way while building this project:
 * it helped draft/format some documentation comments and suggested a few state-stack edge
 * cases to double-check (for example: resetting Match/NetworkManager when returning to the
 * main menu). The team designed the state architecture, implemented the features, and
 * reviewed/edited all AI-suggested text/ideas before integration.
 *
 * Example prompt used:
 * "Review this C++ game state stack (menu/host/client/single-player). Suggest concise Doxygen
 * comments and call out any common edge cases around pushing/popping states and resetting
 * long-lived resources. Do not change runtime behavior."
 */

#include <SFML/Graphics.hpp>
#include "../Input/InputHandler.h"
#include "../Core/Renderer.h"
#include "../Input/AIController.h"
#include <array>
#include <optional>
#include <stdint.h>
#include <stdio.h>
#include <string>

/* Forward declaration */
class GameEngine;

/**
 * @class EngineState
 * @brief Abstract interface for all application states.
 */
class EngineState {
public:
	EngineState() = default;
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

/**
 * @class JoinGameState
 * @brief Runtime IP-entry screen for client LAN play.
 */
class JoinGameState : public EngineState {
public:
	JoinGameState();
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;

private:
	std::string mHostAddressText;

	// Polling-state memory. GameEngine does not currently forward TextEntered
	// events to EngineState, so this state implements simple key-edge detection.
	std::array<bool, 10> mNumberKeyWasDown{};
	std::array<bool, 10> mNumpadKeyWasDown{};
	bool mPeriodKeyWasDown = false;
	bool mBackspaceKeyWasDown = false;
	bool mEnterKeyWasDown = false;
	bool mEscapeKeyWasDown = false;

	void appendDigitIfPressed(int digit);
	void appendPeriodIfPressed();
	void deleteLastCharacterIfPressed();
	bool enterPressedOnce();
	bool escapePressedOnce();
};





/**
 * @class StartMenuState
 * @brief Main menu state that lets the user choose play mode/settings.
 */
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

/**
 * @class SettingsMenuState
 * @brief Basic visual settings menu state.
 */
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

/**
 * @class PauseMenuState
 * @brief Overlay state for pausing gameplay.
 */
class PauseMenuState : public EngineState {
public:
	PauseMenuState();
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
private:
	sf::Texture mPanelTex;
	sf::Texture mExitTex;

	std::optional<sf::Sprite> mPanel;
	std::optional<sf::Sprite> mExitBtn;
	sf::Font mFont;
	sf::RectangleShape mResumeBtn;
	sf::RectangleShape mQuitBtn;
	sf::Text mResumeText;
	//sf::Text mQuitText;

	sf::Text mInstructionsText;
	bool mMouseWasDown = false;
	bool mEscapeWasDown = false;
	bool isMouseOver(const sf::FloatRect& bounds, sf::RenderWindow& window);
};

/**
 * @class ClientPlayingState
 * @brief Non-authoritative network client gameplay state.
 */
class ClientPlayingState : public EngineState {
public:
	ClientPlayingState();
	explicit ClientPlayingState(std::string hostAddressText);
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
private:
	// Runtime host address entered in JoinGameState. This replaces the old flow
	// where players had to edit Config::DEFAULT_HOST_ADDRESS in source code.
	std::string mHostAddressText;

	// Network sockets must be started exactly once per state lifetime.
	// Client binds to an ephemeral local UDP port so it can receive STATE snapshots.
	bool mNetworkStarted = false;

	InputHandler mInputHandler;
	std::uint8_t mMyPlayerID = 255;
	Renderer mRenderer;
	GameStatePacket mLatestState;

	// Once we have at least one STATE snapshot, route future inputs
	// using mLatestState.controlledAwayPlayerId so defensive switching works.
	bool mHaveState = false;
	bool mPauseKeyWasDown = false;

	// Client-side UI timing for the Game Over overlay. The host decides when
	// the match is over; client only delays the local menu art/buttons.
	float mGameOverMenuElapsedSec = 0.f;
	bool mGameOverMouseWasDown = false;
};

/**
 * @class HostPlayingState
 * @brief Authoritative host gameplay state.
 */
class HostPlayingState : public EngineState {
public:
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
private:
	// Host must bind the UDP socket before it can receive JOIN/INPUT.
	bool mNetworkStarted = false;

	InputHandler mInputHandler;
	AIController mAiController;
	std::uint8_t mMyPlayerID = 0;
	Renderer mRenderer;

	// Local UI timer so the final field state remains visible briefly before
	// Game Over buttons cover the screen.
	float mGameOverMenuElapsedSec = 0.f;
	bool mGameOverMouseWasDown = false;
	bool mPauseKeyWasDown = false;
};

/**
 * @class SinglePlayerPlayingState
 * @brief Local-only gameplay state using the same Match/Renderer pipeline.
 */
class SinglePlayerPlayingState : public EngineState {
public:
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;
private:
	InputHandler mInputHandler;
	AIController mAiController;
	std::uint8_t mMyPlayerID = 0;
	Renderer mRenderer;

	// Single-player uses the same delayed Game Over UI behavior as host mode.
	float mGameOverMenuElapsedSec = 0.f;
	bool mGameOverMouseWasDown = false;
	bool mPauseKeyWasDown = false;
};
