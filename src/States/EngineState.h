#pragma once

/**
 * @file EngineState.h
 * @brief Application state classes for menu, gameplay, host, client, and single-player modes.
 *
 * AI disclosure:
 * The host/client gameplay state wiring was implemented and documented with help
 * from OpenAI Codex because it coordinates input, AI, Match, Renderer, and
 * NetworkManager in one frame pipeline.
 *
 * Prompt used:
 * "Help wire an SFML game-state stack into a host-authoritative soccer game.
 * HostPlayingState should build FrameInput, update Match, send GameState, and
 * render snapshots. ClientPlayingState should send input and render latest state.
 * Add a small JoinGameState so users can type the host LAN IP at runtime instead
 * of editing source code."
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
 *
 * SANTI 30/04/26
 * AI disclosure: this small usability state was implemented with Codex help
 * because the architecture/networking already existed and the remaining work
 * was time-crunch UI plumbing for entering a host IP without editing code.
 *
 * Prompt used:
 * "Add a user-friendly Join Game screen to my SFML state stack. Let the user
 * type an IPv4 host address at runtime, press Enter to create ClientPlayingState,
 * press Escape to return to StartMenuState, and keep networking protocol unchanged."
 */
class JoinGameState : public EngineState {
public:
	JoinGameState();
	void tick(GameEngine& engine, float dt) override;
	void render(GameEngine& engine) override;

private:
	std::string mHostAddressText;

	// SANTI 30/04/26
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
	// SANTI 30/04/26
	// Runtime host address entered in JoinGameState. This replaces the old flow
	// where players had to edit Config::DEFAULT_HOST_ADDRESS in source code.
	std::string mHostAddressText;

	// SANTI 28/04/2026: Network sockets must be started exactly once per state lifetime.
	// Client binds to an ephemeral local UDP port so it can receive STATE snapshots.
	bool mNetworkStarted = false;

	InputHandler mInputHandler;
	std::uint8_t mMyPlayerID = 255;
	Renderer mRenderer;
	GameStatePacket mLatestState;

	// SANTI 28/04/2026: Once we have at least one STATE snapshot, we route future inputs
	// using mLatestState.controlledAwayPlayerId so defensive switching works.
	bool mHaveState = false;
	bool mPauseKeyWasDown = false;
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
	// SANTI 28/04/2026: Host must bind the UDP socket before it can receive JOIN/INPUT.
	bool mNetworkStarted = false;

	InputHandler mInputHandler;
	AIController mAiController;
	std::uint8_t mMyPlayerID = 0;
	Renderer mRenderer;
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
};
