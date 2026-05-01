#include "EngineState.h"

/**
 * @file EngineState.cpp
 * @brief Application-state update/render implementation.
 *
 * AI assistance disclosure:
 * A generative AI assistant (Codex) was used in a limited, targeted way to help draft/format some
 * documentation comments and to suggest a few state-transition/input-routing edge cases to
 * double-check (for example: ensuring pause/menu navigation does not leak old match state).
 * The team implemented the state logic and verified behavior via builds and play-testing.
 *
 * Prompt used:
 * "Given this EngineState implementation, suggest places where returning to the main menu should
 * reset Match/NetworkManager, and propose small helper functions.
 * Keep the code readable and do not change behavior."
 */

#include "../Core/GameEngine.h"
#include "../Common/Constants.h"
#include <SFML/Network/IpAddress.hpp>
#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace {
	// Helper to slice one button cell out of the menu sprite sheet.
	// This replaces the old lambda so the constructor stays easy to read.
	static sf::IntRect makeMenuButtonSheetRect(int index, int cellWidth, int cellHeight, int transparentGap) {
		const int x = index * (cellWidth + transparentGap);
		return sf::IntRect({ x, 0 }, { cellWidth, cellHeight });
	}

	// SFML marks Texture::loadFromFile as [[nodiscard]].
	// For menu UI textures, we load "best effort" and keep going if assets are missing.
	static void loadTextureBestEffort(sf::Texture& texture, const char* path) {
		const bool loaded = texture.loadFromFile(path);
		(void)loaded;
	}

	// Helper to apply standard origin/scale/position to an optional button sprite.
	static void setupMenuButtonSprite(
		std::optional<sf::Sprite>& optionalSprite,
		float centerX,
		float centerY,
		float cellWidth,
		float cellHeight,
		float scale)
	{
		if (!optionalSprite) return;

		optionalSprite->setOrigin({ cellWidth * 0.5f, cellHeight * 0.5f });
		optionalSprite->setScale({ scale, scale });
		optionalSprite->setPosition({ centerX, centerY });
	}

	// Shared font for small state-level UI overlays such as Join Game and host
	// networking instructions. Renderer has its own HUD helper, but EngineState
	// also needs text before a match snapshot exists.
	static const sf::Font* getEngineStateUiFont() {
		static sf::Font font;
		static bool loaded = font.openFromFile("assets/fonts/arial.ttf");
		return loaded ? &font : nullptr;
	}

	static void drawStateText(
		sf::RenderWindow& window,
		const sf::Font& font,
		const std::string& textString,
		sf::Vector2f position,
		unsigned int characterSize,
		sf::Color fillColor)
	{
		sf::Text text(font, textString, characterSize);
		text.setFillColor(fillColor);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(2.f);
		text.setPosition(position);
		window.draw(text);
	}

	static bool keyPressedOnce(sf::Keyboard::Key key, bool& keyWasDown) {
		const bool keyIsDown = sf::Keyboard::isKeyPressed(key);
		const bool justPressed = keyIsDown && !keyWasDown;
		keyWasDown = keyIsDown;
		return justPressed;
	}

	static sf::Keyboard::Key numberRowKeyFromDigit(int digit) {
		switch (digit) {
		case 0: return sf::Keyboard::Key::Num0;
		case 1: return sf::Keyboard::Key::Num1;
		case 2: return sf::Keyboard::Key::Num2;
		case 3: return sf::Keyboard::Key::Num3;
		case 4: return sf::Keyboard::Key::Num4;
		case 5: return sf::Keyboard::Key::Num5;
		case 6: return sf::Keyboard::Key::Num6;
		case 7: return sf::Keyboard::Key::Num7;
		case 8: return sf::Keyboard::Key::Num8;
		case 9: return sf::Keyboard::Key::Num9;
		default: return sf::Keyboard::Key::Unknown;
		}
	}

	static sf::Keyboard::Key numpadKeyFromDigit(int digit) {
		switch (digit) {
		case 0: return sf::Keyboard::Key::Numpad0;
		case 1: return sf::Keyboard::Key::Numpad1;
		case 2: return sf::Keyboard::Key::Numpad2;
		case 3: return sf::Keyboard::Key::Numpad3;
		case 4: return sf::Keyboard::Key::Numpad4;
		case 5: return sf::Keyboard::Key::Numpad5;
		case 6: return sf::Keyboard::Key::Numpad6;
		case 7: return sf::Keyboard::Key::Numpad7;
		case 8: return sf::Keyboard::Key::Numpad8;
		case 9: return sf::Keyboard::Key::Numpad9;
		default: return sf::Keyboard::Key::Unknown;
		}
	}

	static std::string localAddressSuggestionText() {
		const std::optional<sf::IpAddress> localAddress =
			sf::IpAddress::getLocalAddress(sf::IpAddress::Type::IpV4);

		if (!localAddress.has_value()) return "Unknown";

		return localAddress->toString();
	}

	static void drawHostNetworkingHint(sf::RenderWindow& window) {
		// Low-risk host usability improvement: SFML can suggest one local IPv4
		// address, but laptops with VPNs/VirtualBox may have multiple adapters.
		// The UI therefore presents it as a suggestion and tells the host what
		// the client needs, without adding platform-specific adapter code.
		const sf::Font* font = getEngineStateUiFont();
		if (!font) return;

		sf::RectangleShape panel(sf::Vector2f(410.f, 86.f));
		panel.setPosition({ 15.f, 92.f });
		panel.setFillColor(sf::Color(0, 0, 0, 145));
		panel.setOutlineColor(sf::Color(220, 220, 180));
		panel.setOutlineThickness(2.f);
		window.draw(panel);

		drawStateText(
			window,
			*font,
			"HOSTING ON PORT " + std::to_string(Config::HOST_PORT),
			{ 25.f, 100.f },
			16,
			sf::Color::White);

		drawStateText(
			window,
			*font,
			"Client should enter: " + localAddressSuggestionText(),
			{ 25.f, 125.f },
			16,
			sf::Color(255, 230, 120));

		drawStateText(
			window,
			*font,
			"If this fails, use Wi-Fi IPv4 from ipconfig.",
			{ 25.f, 150.f },
			14,
			sf::Color(220, 220, 220));
	}

	static void drawClientConnectedHint(sf::RenderWindow& window) {
		// Small confirmation after the host has learned a client endpoint.
		// This replaces the larger connection-instruction box so gameplay view
		// is less cluttered after the client joins.
		const sf::Font* font = getEngineStateUiFont();
		if (!font) return;

		sf::RectangleShape panel(sf::Vector2f(230.f, 34.f));
		panel.setPosition({ 15.f, 92.f });
		panel.setFillColor(sf::Color(0, 0, 0, 130));
		panel.setOutlineColor(sf::Color(120, 230, 140));
		panel.setOutlineThickness(2.f);
		window.draw(panel);

		drawStateText(
			window,
			*font,
			"CLIENT CONNECTED",
			{ 25.f, 98.f },
			16,
			sf::Color(150, 255, 160));
	}

	// GameStatePacket.currentState uses the shared wire contract. State id 3 is
	// GameOverState, so menu/gameplay code can check this without depending on
	// concrete MatchState classes.
	static bool snapshotIsGameOver(const GameStatePacket& gameState) {
		return gameState.currentState == 3;
	}

	static bool gameOverMenuDelayFinished(float elapsedSeconds) {
		return elapsedSeconds >= Config::GAME_OVER_MENU_DELAY_SECONDS;
	}

	static void resetGameOverMenuTracking(float& elapsedSeconds, bool& mouseWasDown) {
		elapsedSeconds = 0.f;
		mouseWasDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	}

	static sf::FloatRect gameOverTryAgainButtonBounds() {
		// TryAgain.png is a 1408x768 canvas with the visible button on the
		// right side of that canvas. Renderer centers/scales the whole canvas,
		// so this rectangle targets the actual visible button, not the full PNG.
		const float buttonWidth = 230.f;
		const float buttonHeight = 115.f;
		const float buttonX = 445.f;
		const float buttonY = 390.f;

		return sf::FloatRect({ buttonX, buttonY }, { buttonWidth, buttonHeight });
	}

	static sf::FloatRect gameOverQuitButtonBounds() {
		// QuitB.png also has transparent canvas padding. These values target the
		// visible quit button after Renderer applies its 0.15 scale.
		const float buttonWidth = 140.f;
		const float buttonHeight = 125.f;
		const float buttonX = 330.f;
		const float buttonY = 375.f;

		return sf::FloatRect({ buttonX, buttonY }, { buttonWidth, buttonHeight });
	}

	static bool pollLeftMouseClickOnce(
		sf::RenderWindow& window,
		bool& mouseWasDown,
		sf::Vector2f& outMouseWorldPosition)
	{
		const bool mouseIsDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
		const bool mouseJustClicked = mouseIsDown && !mouseWasDown;
		mouseWasDown = mouseIsDown;

		if (!mouseJustClicked) return false;

		const sf::Vector2i mousePixelPosition = sf::Mouse::getPosition(window);
		outMouseWorldPosition = window.mapPixelToCoords(mousePixelPosition);
		return true;
	}

	static void resetMatchAndReturnToMainMenu(GameEngine& engine) {
		// Leaving a match should end it completely. Clear Match first, then clear
		// the whole state stack so no old gameplay state remains behind the menu.
		engine.resetNetwork();
		engine.resetMatch();
		engine.resetStateStack(std::make_unique<StartMenuState>());
	}

	static void resetMatchAndRestartSinglePlayer(GameEngine& engine) {
		engine.resetNetwork();
		engine.resetMatch();
		engine.resetStateStack(std::make_unique<SinglePlayerPlayingState>());
	}

	static void resetMatchAndRestartHostMode(GameEngine& engine) {
		engine.resetNetwork();
		engine.resetMatch();
		engine.resetStateStack(std::make_unique<HostPlayingState>());
	}

	static void resetMatchAndReturnToJoinScreen(GameEngine& engine) {
		// A client cannot authoritatively restart the host's match. Try Again
		// returns the client to the Join screen so it can reconnect after the
		// host starts a fresh match.
		engine.resetNetwork();
		engine.resetMatch();
		engine.resetStateStack(std::make_unique<JoinGameState>());
	}

	enum class GameOverRetryTarget {
		SinglePlayer,
		Host,
		ClientJoin
	};

	static void restartGameOverTarget(GameEngine& engine, GameOverRetryTarget retryTarget) {
		if (retryTarget == GameOverRetryTarget::SinglePlayer) {
			resetMatchAndRestartSinglePlayer(engine);
			return;
		}

		if (retryTarget == GameOverRetryTarget::Host) {
			resetMatchAndRestartHostMode(engine);
			return;
		}

		resetMatchAndReturnToJoinScreen(engine);
	}

	static bool updateDelayedGameOverMenu(
		GameEngine& engine,
		float dt,
		float& elapsedSeconds,
		bool& mouseWasDown,
		GameOverRetryTarget retryTarget)
	{
		// Returns true while the active state should stop normal gameplay work.
		// The one-second delay is local UI timing; the authoritative match has
		// already reached GameOver.
		elapsedSeconds += dt;

		if (!gameOverMenuDelayFinished(elapsedSeconds)) return true;

		sf::Vector2f mouseWorldPosition;
		if (!pollLeftMouseClickOnce(engine.getWindow(), mouseWasDown, mouseWorldPosition)) return true;

		if (gameOverTryAgainButtonBounds().contains(mouseWorldPosition)) {
			restartGameOverTarget(engine, retryTarget);
			return true;
		}

		if (gameOverQuitButtonBounds().contains(mouseWorldPosition)) {
			resetMatchAndReturnToMainMenu(engine);
			return true;
		}

		return true;
	}
}

/**************************
* START MENU STATE
**************************/

StartMenuState::StartMenuState() {
	// 1. Background Setup (using the homeMenu art)
	if (mBackgroundTex.loadFromFile("assets/backgrounds/homeMenu.png")) {
		mBackgroundSprite.emplace(mBackgroundTex);
		mBackgroundSprite->setScale({
			static_cast<float>(Config::WINDOW_WIDTH) / mBackgroundTex.getSize().x,
			static_cast<float>(Config::WINDOW_HEIGHT) / mBackgroundTex.getSize().y
			});
	}

	// 2. Refactored Standardized Button Logic
	if (mButtonSheet.loadFromFile("assets/textures/menuButtonsSheet.png")) {
		const int cellW = 200; // Width of one button cell
		const int cellH = 90;  // Height of one button cell
		const int gap = 2;     // Transparent gap between buttons on sheet

		// Construct using SFML 3.1's mandatory-texture constructor
		mBtnPlay.emplace(mButtonSheet, makeMenuButtonSheetRect(0, cellW, cellH, gap));     // Index 0
		mBtnHost.emplace(mButtonSheet, makeMenuButtonSheetRect(1, cellW, cellH, gap));     // Index 1
		mBtnJoin.emplace(mButtonSheet, makeMenuButtonSheetRect(2, cellW, cellH, gap));     // Index 2
		mBtnSettings.emplace(mButtonSheet, makeMenuButtonSheetRect(3, cellW, cellH, gap)); // Index 3

		// 3. Vertical Stack Layout (Centered between player sprites)
		float centerX = Config::WINDOW_WIDTH / 2.0f;
		float startY = 300.0f;   // Moves the whole group up/down
		float vSpacing = 80.0f;  // Fixed gap between button centers
		const float buttonScale = 0.8f;

		// Applying the vertical order: Play -> Join -> Host -> Settings
		setupMenuButtonSprite(mBtnPlay, centerX, startY, static_cast<float>(cellW), static_cast<float>(cellH), buttonScale);
		setupMenuButtonSprite(mBtnJoin, centerX, startY + vSpacing, static_cast<float>(cellW), static_cast<float>(cellH), buttonScale);
		setupMenuButtonSprite(mBtnHost, centerX, startY + vSpacing * 2.f, static_cast<float>(cellW), static_cast<float>(cellH), buttonScale);
		setupMenuButtonSprite(mBtnSettings, centerX, startY + vSpacing * 3.f, static_cast<float>(cellW), static_cast<float>(cellH), buttonScale);
	}
}

void StartMenuState::tick(GameEngine& engine, float dt) {
	auto& window = engine.getWindow();

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
		// Must check if optional has value before dereferencing with *
		if (mBtnPlay && isSpriteClicked(*mBtnPlay, window)) {
			// Every play-mode selection starts a fresh Match. This prevents a
			// previous abandoned/game-over match from leaking score, cows, timer,
			// or ball ownership into the next mode.
			engine.resetNetwork();
			engine.resetMatch();
			engine.transitionTo(std::make_unique<SinglePlayerPlayingState>());
		}
		else if (mBtnHost && isSpriteClicked(*mBtnHost, window)) {
			// Host socket setup belongs in HostPlayingState::tick so the menu
			// only decides navigation. HostPlayingState also renders the runtime
			// IP/port instructions for the user.
			engine.resetNetwork();
			engine.resetMatch();
			engine.transitionTo(std::make_unique<HostPlayingState>());
		}
		else if (mBtnJoin && isSpriteClicked(*mBtnJoin, window)) {
			// JoinGameState lets the user type the host IP at runtime.
			// This replaces hardcoded LAN addresses such as 10.x.x.x.
			engine.resetNetwork();
			engine.resetMatch();
			engine.transitionTo(std::make_unique<JoinGameState>());
		}
		else if (mBtnSettings && isSpriteClicked(*mBtnSettings, window)) {
			engine.transitionTo(std::make_unique<SettingsMenuState>());
		}
	}
}

void StartMenuState::render(GameEngine& engine) {
	auto& window = engine.getWindow();

	// Draw in order: Background first, then buttons
	if (mBackgroundSprite) window.draw(*mBackgroundSprite);
	if (mBtnPlay) window.draw(*mBtnPlay);
	if (mBtnHost) window.draw(*mBtnHost);
	if (mBtnJoin) window.draw(*mBtnJoin);
	if (mBtnSettings) window.draw(*mBtnSettings);
}

bool StartMenuState::isSpriteClicked(sf::Sprite& sprite, sf::RenderWindow& window) {
	sf::Vector2i mousePos = sf::Mouse::getPosition(window);
	// mapPixelToCoords handles scaling/view differences automatically
	sf::Vector2f translatedPos = window.mapPixelToCoords(mousePos);
	return sprite.getGlobalBounds().contains(translatedPos);
}

/**************************
* JOIN GAME STATE
**************************/

JoinGameState::JoinGameState()
	: mHostAddressText(Config::DEFAULT_HOST_ADDRESS)
{
	// Keep the constructor explicit so this state always starts with a useful
	// localhost default for same-machine testing.
}

void JoinGameState::appendDigitIfPressed(int digit) {
	static constexpr std::size_t kMaxIpv4TextLength = 15;
	if (digit < 0) return;
	if (digit > 9) return;

	const std::size_t digitIndex = static_cast<std::size_t>(digit);
	const bool numberRowPressed = keyPressedOnce(
		numberRowKeyFromDigit(digit),
		mNumberKeyWasDown[digitIndex]);
	const bool numpadPressed = keyPressedOnce(
		numpadKeyFromDigit(digit),
		mNumpadKeyWasDown[digitIndex]);

	if (!numberRowPressed && !numpadPressed) return;
	if (mHostAddressText.size() >= kMaxIpv4TextLength) return;

	mHostAddressText.push_back(static_cast<char>('0' + digit));
}

void JoinGameState::appendPeriodIfPressed() {
	static constexpr std::size_t kMaxIpv4TextLength = 15;
	if (!keyPressedOnce(sf::Keyboard::Key::Period, mPeriodKeyWasDown)) return;
	if (mHostAddressText.size() >= kMaxIpv4TextLength) return;

	mHostAddressText.push_back('.');
}

void JoinGameState::deleteLastCharacterIfPressed() {
	if (!keyPressedOnce(sf::Keyboard::Key::Backspace, mBackspaceKeyWasDown)) return;
	if (mHostAddressText.empty()) return;

	mHostAddressText.pop_back();
}

bool JoinGameState::enterPressedOnce() {
	// SFML 3 exposes the keypad Enter as a scancode, not as a Keyboard::Key.
	// Keep this MVP input screen simple and use the standard Enter key.
	return keyPressedOnce(sf::Keyboard::Key::Enter, mEnterKeyWasDown);
}

bool JoinGameState::escapePressedOnce() {
	return keyPressedOnce(sf::Keyboard::Key::Escape, mEscapeKeyWasDown);
}

void JoinGameState::tick(GameEngine& engine, float dt) {
	(void)dt;

	for (int digit = 0; digit <= 9; ++digit) {
		appendDigitIfPressed(digit);
	}

	appendPeriodIfPressed();
	deleteLastCharacterIfPressed();

	if (escapePressedOnce()) {
		engine.transitionTo(std::make_unique<StartMenuState>());
		return;
	}

	if (!enterPressedOnce()) return;

	const std::string hostAddressText =
		mHostAddressText.empty()
		? std::string(Config::DEFAULT_HOST_ADDRESS)
		: mHostAddressText;

	engine.transitionTo(std::make_unique<ClientPlayingState>(hostAddressText));
}

void JoinGameState::render(GameEngine& engine) {
	sf::RenderWindow& window = engine.getWindow();

	sf::RectangleShape background(sf::Vector2f(
		static_cast<float>(Config::WINDOW_WIDTH),
		static_cast<float>(Config::WINDOW_HEIGHT)));
	background.setFillColor(sf::Color(20, 45, 40));
	window.draw(background);

	sf::RectangleShape panel(sf::Vector2f(620.f, 310.f));
	panel.setOrigin({ 310.f, 155.f });
	panel.setPosition({ Config::WINDOW_WIDTH * 0.5f, Config::WINDOW_HEIGHT * 0.5f });
	panel.setFillColor(sf::Color(0, 0, 0, 165));
	panel.setOutlineColor(sf::Color(220, 220, 180));
	panel.setOutlineThickness(3.f);
	window.draw(panel);

	const sf::Font* font = getEngineStateUiFont();
	if (!font) return;

	drawStateText(
		window,
		*font,
		"JOIN NETWORK GAME",
		{ 235.f, 190.f },
		32,
		sf::Color::White);

	drawStateText(
		window,
		*font,
		"Enter host IPv4 address:",
		{ 190.f, 255.f },
		22,
		sf::Color(235, 235, 210));

	sf::RectangleShape inputBox(sf::Vector2f(420.f, 44.f));
	inputBox.setPosition({ 190.f, 290.f });
	inputBox.setFillColor(sf::Color(245, 245, 235));
	inputBox.setOutlineColor(sf::Color::Black);
	inputBox.setOutlineThickness(2.f);
	window.draw(inputBox);

	drawStateText(
		window,
		*font,
		mHostAddressText.empty() ? "_" : mHostAddressText,
		{ 205.f, 298.f },
		24,
		sf::Color::Black);

	drawStateText(
		window,
		*font,
		"Enter = Join    Backspace = Delete    Esc = Back",
		{ 190.f, 365.f },
		18,
		sf::Color(240, 240, 190));

	drawStateText(
		window,
		*font,
		"Local test default: 127.0.0.1",
		{ 190.f, 395.f },
		16,
		sf::Color(200, 220, 220));
}

/**************************
* SETTINGS MENU STATE
**************************/
SettingsMenuState::SettingsMenuState()
{
	loadTextureBestEffort(mBgTex, "assets/backgrounds/homeMenu.png");
	// Use the exact repo folder casing. Windows ignores this, but Linux CI does not.
	loadTextureBestEffort(mPanelTex, "assets/UI/soccer_ui/panel.png");
	loadTextureBestEffort(mSettingWordsTex, "assets/UI/soccer_ui/settings_wording.png");
	loadTextureBestEffort(mExitTex, "assets/UI/soccer_ui/exit.png");

	loadTextureBestEffort(mVolumeHighTex, "assets/UI/soccer_ui/audio_max.png");
	loadTextureBestEffort(mVolumeMidTex, "assets/UI/soccer_ui/audio_mid.png");
	loadTextureBestEffort(mVolumeLowTex, "assets/UI/soccer_ui/audio_low.png");

	loadTextureBestEffort(mBrightnessHighTex, "assets/UI/soccer_ui/brightness_max.png");
	loadTextureBestEffort(mBrightnessMidTex, "assets/UI/soccer_ui/brightness_mid.png");
	loadTextureBestEffort(mBrightnessLowTex, "assets/UI/soccer_ui/brightness_low.png");

	mBg.emplace(mBgTex);
	mBg->setScale({
		static_cast<float>(Config::WINDOW_WIDTH) / mBgTex.getSize().x,
		static_cast<float>(Config::WINDOW_HEIGHT) / mBgTex.getSize().y
		});

	mPanel.emplace(mPanelTex);
	mPanel->setOrigin({
		mPanelTex.getSize().x / 2.0f,
		mPanelTex.getSize().y / 2.0f
		});
	mPanel->setPosition({
		Config::WINDOW_WIDTH / 2.0f,
		Config::WINDOW_HEIGHT / 2.0f
		});
	mPanel->setScale({ 0.9f, 0.9f });

	mSettingWords.emplace(mSettingWordsTex);
	mSettingWords->setOrigin(
		{ mSettingWordsTex.getSize().x / 2.0f,
		mSettingWordsTex.getSize().y / 2.0f });
	mSettingWords->setPosition({ Config::WINDOW_WIDTH / 1.34f, Config::WINDOW_HEIGHT / 4.0f });
	mSettingWords->setScale({ 0.5f, 0.5f });
	mExitBtn.emplace(mExitTex);
	mExitBtn->setPosition({ 500.f, 355.f });
	mExitBtn->setScale({ 0.28f, 0.28f });

	mVolumeIcon.emplace(mVolumeHighTex);
	mVolumeIcon->setPosition({ 200.f, 260.f });
	mVolumeIcon->setScale({ 0.5f, 0.5f });

	mBrightnessIcon.emplace(mBrightnessMidTex);
	mBrightnessIcon->setPosition({ 470.f, 260.f });
	mBrightnessIcon->setScale({ 0.5f, 0.5f });

	updateIcons();
}

void SettingsMenuState::tick(GameEngine& engine, float dt)
{
	auto& window = engine.getWindow();

	bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool justClicked = mouseDown && !mMouseWasDown;

	if (justClicked)
	{
		if (mExitBtn && isMouseOver(mExitBtn->getGlobalBounds(), window)) {
			engine.transitionTo(std::make_unique<StartMenuState>());
			return;
		}

		if (mVolumeIcon && isMouseOver(mVolumeIcon->getGlobalBounds(), window)) {
			mVolumeState = (mVolumeState + 1) % 3;
			updateIcons();
		}

		if (mBrightnessIcon && isMouseOver(mBrightnessIcon->getGlobalBounds(), window)) {
			mBrightnessState = (mBrightnessState + 1) % 3;
			updateIcons();
		}
	}

	mMouseWasDown = mouseDown;
}


void SettingsMenuState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();

	if (mBg) window.draw(*mBg);
	if (mPanel) window.draw(*mPanel);
	if (mVolumeIcon) window.draw(*mVolumeIcon);
	if (mBrightnessIcon) window.draw(*mBrightnessIcon);
	if (mExitBtn) window.draw(*mExitBtn);
	if (mSettingWords) window.draw(*mSettingWords);
}

bool SettingsMenuState::isMouseOver(const sf::FloatRect& bounds, sf::RenderWindow& window)
{
	sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
	sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);

	return bounds.contains(mouseWorld);
}

void SettingsMenuState::updateIcons()
{
	if (mVolumeIcon) {
		if (mVolumeState == 0) {
			mVolumeIcon->setTexture(mVolumeLowTex);
		}
		else if (mVolumeState == 1) {
			mVolumeIcon->setTexture(mVolumeMidTex);
		}
		else {
			mVolumeIcon->setTexture(mVolumeHighTex);
		}
	}

	if (mBrightnessIcon) {
		if (mBrightnessState == 0) {
			mBrightnessIcon->setTexture(mBrightnessLowTex);
		}
		else if (mBrightnessState == 1) {
			mBrightnessIcon->setTexture(mBrightnessMidTex);
		}
		else {
			mBrightnessIcon->setTexture(mBrightnessHighTex);
		}
	}
}
/**************************
* PAUSE MENU STATE
**************************/
PauseMenuState::PauseMenuState()
	: mResumeText(mFont),

	mInstructionsText(mFont)
{
	mExitTex.loadFromFile("assets/UI/soccer_ui/exit.png");
	mPanelTex.loadFromFile("assets/UI/soccer_ui/panel.png");
	mFont.openFromFile("assets/fonts/arial.ttf");

	mPanel.emplace(mPanelTex);
	mPanel->setOrigin({
		mPanelTex.getSize().x / 2.f,
		mPanelTex.getSize().y / 2.f
		});
	mPanel->setPosition({
		Config::WINDOW_WIDTH / 2.f,
		Config::WINDOW_HEIGHT / 2.f
		});
	mPanel->setScale({ 0.8f, 0.8f });

	mInstructionsText.setCharacterSize(18);
	mInstructionsText.setFillColor(sf::Color::White);
	mInstructionsText.setLineSpacing(1.05f);
	mInstructionsText.setString(
		"		 CONTROLS\n\n"
		"Move: W, A, S, D\n"
		"Pass: J\n"
		"Shoot: K\n"
		"Tackle or Steal: L\n"
		"Switch Defender: I"
	);

	sf::FloatRect textBounds = mInstructionsText.getLocalBounds();
	mInstructionsText.setOrigin({
		textBounds.position.x + textBounds.size.x / 2.f,
		textBounds.position.y
		});

	mInstructionsText.setPosition({
		mPanel->getPosition().x,
		mPanel->getPosition().y - 130.f
		});

	mResumeBtn.setSize({ 150.f, 60.f });
	mResumeBtn.setOrigin({ 110.f, 30.f });
	mResumeBtn.setPosition({
		mPanel->getPosition().x + 100.f,
		mPanel->getPosition().y + 100.f
		});
	mResumeBtn.setFillColor(sf::Color(40, 140, 60));

	mExitBtn.emplace(mExitTex);
	mExitBtn->setPosition({ 240.f, 330.f });
	mExitBtn->setScale({ 0.28f, 0.28f });

	mResumeText.setString("RESUME");
	mResumeText.setCharacterSize(28);
	mResumeText.setFillColor(sf::Color::White);
	mResumeText.setPosition({
		mResumeBtn.getPosition().x - 90,
		mResumeBtn.getPosition().y - 18.f
		});

	// If Escape opened the pause menu, that same physical key press should not
	// immediately close it on the next frame. Store the current key state so the
	// menu only closes after the player releases Escape and presses it again.
	mEscapeWasDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
}

void PauseMenuState::tick(GameEngine& engine, float dt)
{
	auto& window = engine.getWindow();

	bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool justClicked = mouseDown && !mMouseWasDown;

	if (justClicked) {
		if (isMouseOver(mResumeBtn.getGlobalBounds(), window)) {
			engine.popState();
			return;
		}

		if (mExitBtn && isMouseOver(mExitBtn->getGlobalBounds(), window)) {
			// Exit means "end this match", not just "hide the pause menu".
			// resetStateStack removes the paused gameplay state underneath.
			resetMatchAndReturnToMainMenu(engine);
			return;
		}
	}

	bool escapeDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);

	if (escapeDown && !mEscapeWasDown) {
		engine.popState();
		return;
	}

	mEscapeWasDown = escapeDown;

	mMouseWasDown = mouseDown;
}

void PauseMenuState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();

	/* Draw semi-transparent overlay (DO NOT call window.clear()) */
	sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(Config::WINDOW_WIDTH), static_cast<float>(Config::WINDOW_HEIGHT)));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));  /* Black with 180 alpha */
	window.draw(overlay);
	if (mPanel) window.draw(*mPanel);

	window.draw(mInstructionsText);
	window.draw(mResumeBtn);
	//window.draw(mQuitBtn);
	window.draw(mResumeText);
	if (mExitBtn) window.draw(*mExitBtn);


}
bool PauseMenuState::isMouseOver(const sf::FloatRect& bounds, sf::RenderWindow& window)
{
	sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
	sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);
	return bounds.contains(mouseWorld);
}

/**************************
* CLIENT PLAYING STATE
**************************/

ClientPlayingState::ClientPlayingState()
	: ClientPlayingState(Config::DEFAULT_HOST_ADDRESS)
{
}

ClientPlayingState::ClientPlayingState(std::string hostAddressText)
	: mHostAddressText(std::move(hostAddressText))
{
	// Host address is supplied by JoinGameState so users do not have to edit
	// Constants.h before starting a LAN client.
}

void ClientPlayingState::tick(GameEngine& engine, float dt)
{
	auto& network = engine.getNetwork();

	// Start the client socket exactly once.
	// For LAN play, JoinGameState passes the host IPv4 address typed by the user.
	if (!mNetworkStarted) {
		// The Join screen is intentionally IPv4-only for MVP simplicity.
		// If parsing fails, fall back to localhost so local tests remain easy.
		const std::optional<sf::IpAddress> resolved =
			sf::IpAddress::fromString(mHostAddressText);

		const sf::IpAddress hostAddress = resolved.has_value()
			? *resolved
			: sf::IpAddress::LocalHost;

		network.startClient(hostAddress, Config::HOST_PORT);
		mNetworkStarted = true;
	}

	/* Check for pause */
	/*if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}*/
	bool pauseKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	if (pauseKeyDown && !mPauseKeyWasDown) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	mPauseKeyWasDown = pauseKeyDown;

	/* Handshake phase : request player ID if we don't have one yet */
	if (mMyPlayerID == 255) {
		network.sendJoinRequest();

		if (!network.pollIdAssignment(mMyPlayerID)) {
			return; /* Still waiting for assignment */
		}

		std::cout << "Assigned Player ID: " << static_cast<int>(mMyPlayerID) << std::endl;
	}

	/* Normal gameplay : send input to host */
	// After we receive the first snapshot, route inputs using the
	// host-authoritative controlledAwayPlayerId (enables defensive switching).
	std::uint8_t inputPlayerId = mMyPlayerID;
	if (mHaveState) {
		inputPlayerId = mLatestState.controlledAwayPlayerId;
	}

	// IMPORTANT for local testing with 2 instances on the same PC.
	//
	// InputHandler uses sf::Keyboard::isKeyPressed which reads the GLOBAL keyboard state,
	// even if a window is out of focus. This means if you run a Host window and a Client
	// window on the same machine, BOTH processes would read the same key presses and it
	// would feel like you're "controlling both teams at once".
	//
	// Fix: only the focused window is allowed to generate real input. The unfocused window
	// sends a neutral packet (no movement, no buttons). This does NOT affect real LAN play,
	// because host and client are on different machines with different keyboards.
	InputPacket clientInput{};
	clientInput.playerId = inputPlayerId;

	const bool windowHasFocus = engine.getWindow().hasFocus();
	if (windowHasFocus) {
		clientInput = mInputHandler.getLocalInput(inputPlayerId);
	}
	network.sendPlayerInput(clientInput);

	/* Receive and apply latest game state from host */
	GameStatePacket latestGameState;
	if (network.receiveLatestGameState(latestGameState)) {
		mLatestState = latestGameState; /* Store for rendering */
		mHaveState = true;
	}

	if (mHaveState && snapshotIsGameOver(mLatestState)) {
		(void)updateDelayedGameOverMenu(
			engine,
			dt,
			mGameOverMenuElapsedSec,
			mGameOverMouseWasDown,
			GameOverRetryTarget::ClientJoin);
		return;
	}

	resetGameOverMenuTracking(mGameOverMenuElapsedSec, mGameOverMouseWasDown);
}

void ClientPlayingState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();

	const bool showGameOverOverlay =
		!snapshotIsGameOver(mLatestState) ||
		gameOverMenuDelayFinished(mGameOverMenuElapsedSec);

	mRenderer.render(window, mLatestState, true, showGameOverOverlay);
}

/**************************
* HOST PLAYING STATE
**************************/

void HostPlayingState::tick(GameEngine& engine, float dt)
{
	auto& network = engine.getNetwork();
	auto& match = engine.getMatch();

	// Start the host socket exactly once (bind to fixed port).
	if (!mNetworkStarted) {
		network.startHost(Config::HOST_PORT);
		mNetworkStarted = true;

		// For single-client MVP, host controls Home player 0.
		// Client is assigned Away player 4 by handshake (see NetworkManager).
		match.setControlledPlayerIds(0, 4);
	}

	GameStatePacket stateBeforeTick;
	match.getGameState(stateBeforeTick);

	if (snapshotIsGameOver(stateBeforeTick)) {
		// Host keeps broadcasting the final snapshot while the delayed Game
		// Over menu is active, so a connected client can also show the result.
		network.sendGameState(stateBeforeTick);

		(void)updateDelayedGameOverMenu(
			engine,
			dt,
			mGameOverMenuElapsedSec,
			mGameOverMouseWasDown,
			GameOverRetryTarget::Host);
		return;
	}

	resetGameOverMenuTracking(mGameOverMenuElapsedSec, mGameOverMouseWasDown);

	/* Check for pause */
	const bool pauseKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	if (pauseKeyDown && !mPauseKeyWasDown) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}
	mPauseKeyWasDown = pauseKeyDown;

	/* Handle any new clients trying to join */
	// pollIncomingInputs() handles JOIN_REQUEST (handshake) and INPUT packets in
	// one place, so HostPlayingState relies on it exclusively.

	// Host input is always for the currently controlled HOME player.
	// This enables defensive switching and "control the ball owner" without changing networking.
	const std::uint8_t hostControlledId = match.getControlledHomePlayerId();

	FrameInput frameData{};

	// Same local-two-instances fix as the client side.
	// Only the focused window is allowed to generate real keyboard input.
	const bool windowHasFocus = engine.getWindow().hasFocus();
	if (windowHasFocus) {
		frameData.inputs[hostControlledId] = mInputHandler.getLocalInput(hostControlledId);
	}
	frameData.inputs[hostControlledId].playerId = hostControlledId;

	/* Track human player slots */
	bool isHuman[Config::kNumPlayers] = { false };
	isHuman[hostControlledId] = true;

	// Reserve the away-side controlled player for the client,
	// even if no INPUT has arrived yet. This prevents AI from moving the client's
	// player during handshake / packet loss.
	const std::uint8_t awayControlledId = match.getControlledAwayPlayerId();
	if (awayControlledId < Config::kNumPlayers) {
		isHuman[awayControlledId] = true;
		frameData.inputs[awayControlledId].playerId = awayControlledId;
	}

	/* Collect client packets */
	std::queue<InputPacket> remotePackets;
	network.pollIncomingInputs(remotePackets);

	/* Process incoming packets */
	while (!remotePackets.empty()) {
		InputPacket p = remotePackets.front();
		remotePackets.pop();

		if (p.playerId >= Config::kNumPlayers) {
			std::cout << "Received packet with invalid player ID: " << static_cast<int>(p.playerId) << std::endl;
			continue;
		}

		if (p.playerId != awayControlledId) {
			// Networking is single-client for this MVP. The client is allowed to
			// control only the host-authoritative Away controlled slot. This
			// prevents a bad/stale packet from accidentally moving Home players
			// or an AI-controlled Away teammate.
			std::cout
				<< "Ignored remote input for non-client player ID: "
				<< static_cast<int>(p.playerId)
				<< " expected "
				<< static_cast<int>(awayControlledId)
				<< std::endl;
			continue;
		}

		frameData.inputs[p.playerId] = p;
		isHuman[p.playerId] = true;
	}

	/* Get current game state for AI */
	GameStatePacket currentState;
	match.getGameState(currentState);

	/* Fill gaps with packets from AI */
	for (uint8_t i = 0; i < Config::kNumPlayers; ++i) {
		if (!isHuman[i]) {
			/* Pass player ID and game state packet to AI */
			frameData.inputs[i] = mAiController.getAIInput(i, currentState, dt);
		}
	}

	/* Update the match */
	match.update(frameData, dt);

	/* Get updated game state and send to clients */
	match.getGameState(currentState);
	network.sendGameState(currentState);
}

void HostPlayingState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();
	auto& match = engine.getMatch();

	/* Get current game state and render */
	GameStatePacket currentState;
	match.getGameState(currentState);

	const bool showGameOverOverlay =
		!snapshotIsGameOver(currentState) ||
		gameOverMenuDelayFinished(mGameOverMenuElapsedSec);

	mRenderer.render(window, currentState, true, showGameOverOverlay);

	if (snapshotIsGameOver(currentState)) return;

	// Show connection instructions only while waiting for the client.
	// Once NetworkManager has learned a remote endpoint, replace the big box
	// with a small confirmation so it does not cover gameplay.
	if (!engine.getNetwork().hasRemoteClient()) {
		drawHostNetworkingHint(window);
		return;
	}

	drawClientConnectedHint(window);
}

/**************************
* SINGLEPLAYER STATE
**************************/

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt)
{
	auto& match = engine.getMatch();

	GameStatePacket stateBeforeTick;
	match.getGameState(stateBeforeTick);

	if (snapshotIsGameOver(stateBeforeTick)) {
		(void)updateDelayedGameOverMenu(
			engine,
			dt,
			mGameOverMenuElapsedSec,
			mGameOverMouseWasDown,
			GameOverRetryTarget::SinglePlayer);
		return;
	}

	resetGameOverMenuTracking(mGameOverMenuElapsedSec, mGameOverMouseWasDown);

	/* Check for pause */
	const bool pauseKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	if (pauseKeyDown && !mPauseKeyWasDown) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}
	mPauseKeyWasDown = pauseKeyDown;

	/* Build frame input */
	// Singleplayer uses the same control policy as host play.
	// You always send input for match.getControlledHomePlayerId().
	const std::uint8_t myId = match.getControlledHomePlayerId();

	FrameInput frameData{};
	frameData.inputs[myId] = mInputHandler.getLocalInput(myId);

	/* Get current game state for AI */
	GameStatePacket currentState;
	match.getGameState(currentState);

	/* Fill remaining slots with AI */
	for (uint8_t i = 0; i < Config::kNumPlayers; ++i) {
		if (i == myId) continue;
		frameData.inputs[i] = mAiController.getAIInput(i, currentState, dt);
	}

	/* Update match */
	match.update(frameData, dt);
}

void SinglePlayerPlayingState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();
	auto& match = engine.getMatch();

	/* Get current game state and render */
	GameStatePacket currentState;
	match.getGameState(currentState);

	// Single-player controls only the Home side. Hide the Away controlled-player
	// marker so the player does not think the AI opponent is also user-controlled.
	const bool showGameOverOverlay =
		!snapshotIsGameOver(currentState) ||
		gameOverMenuDelayFinished(mGameOverMenuElapsedSec);

	mRenderer.render(window, currentState, false, showGameOverOverlay);
}
