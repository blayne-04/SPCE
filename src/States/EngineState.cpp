#include "EngineState.h"
#include "../Core/GameEngine.h"
#include "../Common/Constants.h"
#include <iostream>
#include <memory>
#include <optional> // SANTI 29/04/26: for resolving Config::DEFAULT_HOST_ADDRESS

namespace {
	// SANTI 30/04/26: Helper to slice one button cell out of the menu sprite sheet.
	// This replaces the old lambda so the constructor stays easy to read.
	static sf::IntRect makeMenuButtonSheetRect(int index, int cellWidth, int cellHeight, int transparentGap) {
		const int x = index * (cellWidth + transparentGap);
		return sf::IntRect({ x, 0 }, { cellWidth, cellHeight });
	}

	// SANTI 30/04/26: SFML marks Texture::loadFromFile as [[nodiscard]].
	// For menu UI textures, we load "best effort" and keep going if assets are missing.
	static void loadTextureBestEffort(sf::Texture& texture, const char* path) {
		const bool loaded = texture.loadFromFile(path);
		(void)loaded;
	}

	// SANTI 30/04/26: Helper to apply standard origin/scale/position to an optional button sprite.
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
			engine.transitionTo(std::make_unique<SinglePlayerPlayingState>());
		}
		else if (mBtnHost && isSpriteClicked(*mBtnHost, window)) {
			engine.transitionTo(std::make_unique<HostPlayingState>());
		}
		else if (mBtnJoin && isSpriteClicked(*mBtnJoin, window)) {
			engine.transitionTo(std::make_unique<ClientPlayingState>());
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
* SETTINGS MENU STATE
**************************/
SettingsMenuState::SettingsMenuState()
{
	loadTextureBestEffort(mBgTex, "assets/backgrounds/homeMenu.png");
	loadTextureBestEffort(mPanelTex, "assets/ui/soccer_ui/panel.png");
	loadTextureBestEffort(mSettingWordsTex, "assets/ui/soccer_ui/settings_wording.png");
	loadTextureBestEffort(mExitTex, "assets/ui/soccer_ui/exit.png");

	loadTextureBestEffort(mVolumeHighTex, "assets/ui/soccer_ui/audio_max.png");
	loadTextureBestEffort(mVolumeMidTex, "assets/ui/soccer_ui/audio_mid.png");
	loadTextureBestEffort(mVolumeLowTex, "assets/ui/soccer_ui/audio_low.png");

	loadTextureBestEffort(mBrightnessHighTex, "assets/ui/soccer_ui/brightness_max.png");
	loadTextureBestEffort(mBrightnessMidTex, "assets/ui/soccer_ui/brightness_mid.png");
	loadTextureBestEffort(mBrightnessLowTex, "assets/ui/soccer_ui/brightness_low.png");

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
	mQuitText(mFont)
{
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

	mResumeBtn.setSize({ 220.f, 60.f });
	mResumeBtn.setOrigin({ 110.f, 30.f });
	mResumeBtn.setPosition({ Config::WINDOW_WIDTH / 2.f, Config::WINDOW_HEIGHT / 2.f - 40.f });
	mResumeBtn.setFillColor(sf::Color(40, 140, 60));

	mQuitBtn.setSize({ 220.f, 60.f });
	mQuitBtn.setOrigin({ 110.f, 30.f });
	mQuitBtn.setPosition({ Config::WINDOW_WIDTH / 2.f, Config::WINDOW_HEIGHT / 2.f + 50.f });
	mQuitBtn.setFillColor(sf::Color(160, 50, 50));

	mResumeText.setString("RESUME");
	mResumeText.setCharacterSize(28);
	mResumeText.setFillColor(sf::Color::White);
	mResumeText.setPosition({ Config::WINDOW_WIDTH / 2.f - 55.f, Config::WINDOW_HEIGHT / 2.f - 58.f });

	mQuitText.setString("QUIT");
	mQuitText.setCharacterSize(28);
	mQuitText.setFillColor(sf::Color::White);
	mQuitText.setPosition({ Config::WINDOW_WIDTH / 2.f - 35.f, Config::WINDOW_HEIGHT / 2.f + 32.f });
}

void PauseMenuState::tick(GameEngine& engine, float dt)
{
	/* TODO: Implement input handling for pause menu */
	auto& window = engine.getWindow();

	bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool justClicked = mouseDown && !mMouseWasDown;

	if (justClicked) {
		if (isMouseOver(mResumeBtn.getGlobalBounds(), window)) {
			engine.popState();
			return;
		}

		if (isMouseOver(mQuitBtn.getGlobalBounds(), window)) {
			engine.transitionTo(std::make_unique<StartMenuState>());
			return;
		}
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.popState();
		return;
	}

	mMouseWasDown = mouseDown;
}

void PauseMenuState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();

	/* Draw semi-transparent overlay (DO NOT call window.clear()) */
	sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(Config::WINDOW_WIDTH), static_cast<float>(Config::WINDOW_HEIGHT)));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));  /* Black with 180 alpha */
	window.draw(overlay);
	/* TODO: Draw pause menu options */
	if (mPanel) window.draw(*mPanel);

	window.draw(mResumeBtn);
	window.draw(mQuitBtn);
	window.draw(mResumeText);
	window.draw(mQuitText);

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

void ClientPlayingState::tick(GameEngine& engine, float dt)
{
	auto& network = engine.getNetwork();

	// SANTI 28/04/2026: Start the client socket exactly once.
	// For LAN play, change Config::DEFAULT_HOST_ADDRESS in Common/Constants.h.
	if (!mNetworkStarted) {
		// SANTI 29/04/26: SFML 3 removed the IpAddress(string) constructor.
		// Resolve from our config string. If resolution fails, fall back to localhost
		// so the game still runs in single-machine tests.
		const std::optional<sf::IpAddress> resolved =
			sf::IpAddress::resolve(Config::DEFAULT_HOST_ADDRESS);

		const sf::IpAddress hostAddress = resolved.has_value()
			? *resolved
			: sf::IpAddress::LocalHost;

		network.startClient(hostAddress, Config::HOST_PORT);
		mNetworkStarted = true;
	}

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	/* Handshake phase : request player ID if we don't have one yet */
	if (mMyPlayerID == 255) {
		network.sendJoinRequest();

		if (!network.pollIdAssignment(mMyPlayerID)) {
			return; /* Still waiting for assignment */
		}

		std::cout << "Assigned Player ID: " << static_cast<int>(mMyPlayerID) << std::endl;
	}

	/* Normal gameplay : send input to host */
	// SANTI 28/04/2026: After we receive the first snapshot, route inputs using the
	// host-authoritative controlledAwayPlayerId (enables defensive switching).
	std::uint8_t inputPlayerId = mMyPlayerID;
	if (mHaveState) {
		inputPlayerId = mLatestState.controlledAwayPlayerId;
	}

	// SANTI 29/04/26: IMPORTANT for local testing with 2 instances on the same PC.
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
}

void ClientPlayingState::render(GameEngine& engine)
{
	auto& window = engine.getWindow();
	mRenderer.render(window, mLatestState);
}

/**************************
* HOST PLAYING STATE
**************************/

void HostPlayingState::tick(GameEngine& engine, float dt)
{
	auto& network = engine.getNetwork();
	auto& match = engine.getMatch();

	// SANTI 28/04/2026: Start the host socket exactly once (bind to fixed port).
	if (!mNetworkStarted) {
		network.startHost(Config::HOST_PORT);
		mNetworkStarted = true;

		// SANTI 28/04/2026: For single-client MVP, host controls Home player 0.
		// Client is assigned Away player 4 by handshake (see NetworkManager).
		match.setControlledPlayerIds(0, 4);
	}

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	/* Handle any new clients trying to join */
	// SANTI 29/04/26: Do NOT call handleHandshakeRequests() here.
	//
	// Reason:
	// - handleHandshakeRequests() drains the UDP socket and only processes JOIN_REQUEST.
	// - Any INPUT packets received during that drain would be discarded.
	//
	// pollIncomingInputs() already handles JOIN_REQUEST (and INPUT) correctly in one place,
	// so HostPlayingState should rely on pollIncomingInputs() only.
	//
	// This makes networking behavior stable and avoids "missing client input" glitches.

	// SANTI 28/04/2026: Host input is always for the currently controlled HOME player.
	// This enables defensive switching and "control the ball owner" without changing networking.
	const std::uint8_t hostControlledId = match.getControlledHomePlayerId();

	FrameInput frameData{};

	// SANTI 29/04/26: Same local-two-instances fix as the client side.
	// Only the focused window is allowed to generate real keyboard input.
	const bool windowHasFocus = engine.getWindow().hasFocus();
	if (windowHasFocus) {
		frameData.inputs[hostControlledId] = mInputHandler.getLocalInput(hostControlledId);
	}
	frameData.inputs[hostControlledId].playerId = hostControlledId;

	/* Track human player slots */
	bool isHuman[Config::kNumPlayers] = { false };
	isHuman[hostControlledId] = true;

	// SANTI 28/04/2026: Reserve the away-side controlled player for the client,
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
	mRenderer.render(window, currentState);
}

/**************************
* SINGLEPLAYER STATE
**************************/

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt)
{
	auto& match = engine.getMatch();

	/* Check for pause */
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	/* Build frame input */
	// SANTI 28/04/2026: Singleplayer uses the same control policy as host play.
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
	mRenderer.render(window, currentState);
}
