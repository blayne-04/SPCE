#include "Renderer.h"

/**
 * @file Renderer.cpp
 * @brief Snapshot-only rendering implementation.
 *
 * AI assistance disclosure:
 * A generative AI assistant (DeepSeek) was used in a limited way to help extract small helper
 * functions (sprite-sheet frame selection, layered sprite drawing) and to draft some
 * documentation comments. The renderer behavior (what is drawn and from which packet
 * fields) was implemented and reviewed by the team and validated via play-testing.
 *
 * Prompt used:
 * "Review this C++/SFML renderer implementation. Suggest a readable structure for drawing
 * a snapshot-only scene (players, ball, cows, HUD, overlays) from GameStatePacket, and help
 * reduce duplication in sprite-sheet frame selection without changing runtime behavior."
 */

#include "Common/Constants.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>

namespace {
	// Loads the HUD font once and reuses it for every frame.
	// Returns nullptr if the font is missing so HUD rendering can fail safely.
	const sf::Font* getHudFont() {
		static sf::Font font;
		static bool loaded = font.openFromFile("assets/fonts/arial.ttf");
		return loaded ? &font : nullptr;
	}

	// Converts the packet's numeric match-state ID into a readable HUD label.
	std::string stateNameFromId(int stateId) {
		switch (stateId) {
		case 0: return "Kickoff";
		case 1: return "Playing";
		case 2: return "Goal";
		case 3: return "GameOver";
		default: return "Unknown";
		}
	}

	// Formats the match clock as M:SS. The timer counts up in GameStatePacket.
	std::string formatClock(float seconds) {
		int total = std::max(0, static_cast<int>(seconds));
		std::ostringstream oss;
		oss << (total / 60) << ":" << std::setfill('0') << std::setw(2) << (total % 60);
		return oss.str();
	}

	// HUD text helper (no lambdas).
	static void drawHudText(
		sf::RenderWindow& window,
		const sf::Font& font,
		const std::string& textString,
		sf::Vector2f position,
		sf::Color color,
		int characterSize)
	{
		sf::Text text(font, textString, characterSize);
		text.setFillColor(color);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(2.f);
		text.setPosition(position);
		window.draw(text);
	}

	// Centered text helper for the Game Over overlay. Kept in Renderer because
	// this is presentation-only; Match still owns score and winner data.
	static void drawCenteredHudText(
		sf::RenderWindow& window,
		const sf::Font& font,
		const std::string& textString,
		float centerX,
		float topY,
		sf::Color color,
		int characterSize)
	{
		sf::Text text(font, textString, characterSize);
		text.setFillColor(color);
		text.setOutlineColor(sf::Color::Black);
		text.setOutlineThickness(2.f);

		const sf::FloatRect localBounds = text.getLocalBounds();
		text.setOrigin({
			localBounds.position.x + localBounds.size.x * 0.5f,
			localBounds.position.y
			});
		text.setPosition({ centerX, topY });

		window.draw(text);
	}

	// Converts the final score into a readable winner line for the Game Over
	// menu. Renderer only reads the packet; it does not decide match rules.
	static std::string winnerTextFromGameState(const GameStatePacket& gameState) {
		if (gameState.scoreHome > gameState.scoreAway) return "HOME WINS";
		if (gameState.scoreAway > gameState.scoreHome) return "AWAY WINS";
		return "DRAW";
	}

	static std::string finalScoreTextFromGameState(const GameStatePacket& gameState) {
		return
			"Final Score: Home " +
			std::to_string(gameState.scoreHome) +
			" - " +
			std::to_string(gameState.scoreAway) +
			" Away";
	}

	// ------------------------------------------------------------------------
	// COW AND BALL SPRITESHEET ANIMATION
	// ------------------------------------------------------------------------
	// The host simulates positions and velocities. Renderer chooses animation
	// frames locally from the authoritative snapshot; no packet fields are needed.
	static constexpr int kCowWalkSheetColumns = 8;
	static constexpr int kCowWalkSheetRows = 4;
	static constexpr float kCowWalkSecondsPerFrame = 0.11f;

	static constexpr int kCowIdleSheetColumns = 13;
	static constexpr int kCowIdleSheetRows = 3;
	static constexpr int kCowIdleFrameCount = 36;
	static constexpr float kCowIdleSecondsPerFrame = 0.09f;

	static constexpr int kBallIconSheetColumns = 4;
	static constexpr int kBallIconSheetRows = 2;
	static constexpr int kBallIconRow = 1; // second row, zero-based
	static constexpr int kControlledPlayerIndicatorColumn = 0; // first column, zero-based
	static constexpr int kBallIconFirstColumn = 1; // second column, zero-based
	static constexpr int kBallIconFrameCount = 3;
	static constexpr float kBallSecondsPerFrame = 0.075f;

	static constexpr int kPlayerDirectionCount = 4;
	static constexpr int kPlayerSkinColorCount = 9;
	static constexpr int kFirstRealisticSkinColor = 2;
	static constexpr int kTeamCount = 2;
	static constexpr int kHomeTeamIndex = 0;
	static constexpr int kAwayTeamIndex = 1;
	static constexpr int kHomeWhitePantsColor = 1;
	static constexpr int kAwayBlackPantsColor = 3;
	static constexpr int kGoalkeeperBlackUniformColor = 3;
	static constexpr int kWhiteOutfieldShirtColor = 1;
	static constexpr int kApprovedShirtColorCount = 8;

	// These are the outfield shirt folders still approved in assets.
	// Color3 is intentionally excluded because goalkeepers own the all-black kit.
	// This prevents an outfield team from randomly wearing black shirts with
	// black pants and looking like goalkeepers.
	static constexpr std::array<int, kApprovedShirtColorCount> kApprovedShirtColors = {
		1, 2, 4, 5, 6, 7, 9, 10
	};

	// Creates a local renderer RNG for visual-only randomization.
	// This is intentionally not part of GameStatePacket because shirt/skin
	// randomization does not affect gameplay or host-authoritative simulation.
	static std::mt19937 makeRendererRandomGenerator() {
		const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		const unsigned int timeSeed = static_cast<unsigned int>(now);
		const unsigned int deviceSeed = std::random_device{}();
		return std::mt19937(timeSeed ^ (deviceSeed + 0x9e3779b9u));
	}

	// Maps renderer direction indexes to the exact sprite filenames on disk.
	static std::string playerDirectionFileName(int directionIndex) {
		// Direction order used by Renderer:
		// 0 = Down, 1 = Up, 2 = Left, 3 = Right.
		// The asset folder spells "RIght.png" with a capital I, so keep that
		// exact spelling for better cross-platform behavior.
		switch (directionIndex) {
		case 0: return "Down.png";
		case 1: return "Up.png";
		case 2: return "Left.png";
		case 3: return "RIght.png";
		default: return "Down.png";
		}
	}

	// Selects one randomized skin-folder number for each player slot.
	// Player IDs keep stable skin colors for the lifetime of this Renderer.
	static std::array<int, Config::kNumPlayers> randomizedPlayerSkinColors() {
		std::array<int, kPlayerSkinColorCount> availableSkinColors{};

		for (int i = 0; i < kPlayerSkinColorCount; ++i) {
			availableSkinColors[i] = kFirstRealisticSkinColor + i;
		}

		// Shuffle the realistic skin folders once per Renderer construction.
		// This avoids every player using the same CharacterSheets color while
		// keeping each player's selected skin stable during the match.
		std::mt19937 randomGenerator = makeRendererRandomGenerator();
		std::shuffle(availableSkinColors.begin(), availableSkinColors.end(), randomGenerator);

		std::array<int, Config::kNumPlayers> playerSkinColors{};

		for (std::size_t playerIndex = 0; playerIndex < Config::kNumPlayers; ++playerIndex) {
			playerSkinColors[playerIndex] = availableSkinColors[playerIndex % availableSkinColors.size()];
		}

		return playerSkinColors;
	}

	// Converts Color2, Color3, etc. folder numbers into folder names.
	static std::string skinFolderNameFromColorNumber(int skinColorNumber) {
		return "Color" + std::to_string(skinColorNumber);
	}

	// Builds a full path for one direction sheet in a ColorN folder.
	static std::string spriteSheetPath(
		const std::string& basePath,
		int colorNumber,
		int directionIndex)
	{
		return
			basePath +
			skinFolderNameFromColorNumber(colorNumber) +
			"/" +
			playerDirectionFileName(directionIndex);
	}

	// Uses packet teamId when valid, and falls back to the player ID contract
	// when a packet has an unexpected teamId.
	static int safeTeamIndexFromPlayerState(const PlayerState& playerState, std::size_t playerIndex) {
		if (playerState.teamId < kTeamCount) return static_cast<int>(playerState.teamId);
		return (playerIndex < 4) ? kHomeTeamIndex : kAwayTeamIndex;
	}

	// Loads one body/skin texture from CharacterSheets.
	static bool loadPlayerSkinTexture(
		sf::Texture& destinationTexture,
		const std::string& basePath,
		int skinColorNumber,
		int directionIndex)
	{
		const std::string fullPath = spriteSheetPath(basePath, skinColorNumber, directionIndex);

		return destinationTexture.loadFromFile(fullPath);
	}

	// Loads all body/skin direction sheets for every player ID.
	// Missing randomized skin files fall back to Color2 so players still render.
	static void loadPlayerSkinTextures(
		std::array<std::array<sf::Texture, 4>, Config::kNumPlayers>& playerSkinTextures)
	{
		const std::string basePath = "assets/textures/Penzilla Protagonist/CharacterSheets/";
		const std::array<int, Config::kNumPlayers> selectedSkinColors = randomizedPlayerSkinColors();

		for (std::size_t playerIndex = 0; playerIndex < Config::kNumPlayers; ++playerIndex) {
			const int skinColorNumber = selectedSkinColors[playerIndex];

			for (int directionIndex = 0; directionIndex < kPlayerDirectionCount; ++directionIndex) {
				const bool loaded = loadPlayerSkinTexture(
					playerSkinTextures[playerIndex][directionIndex],
					basePath,
					skinColorNumber,
					directionIndex);

				// Best-effort fallback: if one randomized folder is missing a
				// direction file, try Color2 so the game can still render players.
				if (loaded) continue;

				const bool fallbackLoaded = loadPlayerSkinTexture(
					playerSkinTextures[playerIndex][directionIndex],
					basePath,
					kFirstRealisticSkinColor,
					directionIndex);
				(void)fallbackLoaded;
			}
		}
	}

	// Loads one shirt or pants texture from ShirtSheets/PantSheets.
	static bool loadUniformTexture(
		sf::Texture& destinationTexture,
		const std::string& basePath,
		int colorNumber,
		int directionIndex)
	{
		const std::string fullPath = spriteSheetPath(basePath, colorNumber, directionIndex);

		return destinationTexture.loadFromFile(fullPath);
	}

	// Verifies that a ColorN uniform folder has all four direction files before
	// it is selected for a team uniform.
	static bool uniformFolderExistsForAllDirections(const std::string& basePath, int colorNumber) {
		for (int directionIndex = 0; directionIndex < kPlayerDirectionCount; ++directionIndex) {
			const std::string fullPath = spriteSheetPath(basePath, colorNumber, directionIndex);

			if (!std::filesystem::exists(fullPath)) return false;
		}

		return true;
	}

	// Returns approved outfield shirt colors in randomized order.
	// Color3 is excluded globally because it belongs to goalkeepers.
	static std::array<int, kApprovedShirtColorCount> shuffledApprovedShirtColors() {
		std::array<int, kApprovedShirtColorCount> randomizedShirtColors = kApprovedShirtColors;
		std::mt19937 randomGenerator = makeRendererRandomGenerator();
		std::shuffle(randomizedShirtColors.begin(), randomizedShirtColors.end(), randomGenerator);
		return randomizedShirtColors;
	}

	// Picks the first existing shirt color from a shuffled list.
	static int firstAvailableShirtColorFromShuffledList(
		const std::array<int, kApprovedShirtColorCount>& randomizedShirtColors,
		const std::string& shirtBasePath)
	{
		for (int shirtColor : randomizedShirtColors) {
			if (!uniformFolderExistsForAllDirections(shirtBasePath, shirtColor)) continue;
			return shirtColor;
		}

		return kApprovedShirtColors[0];
	}

	// Picks an away shirt that exists and differs from the home shirt.
	// If only one folder is available, it returns the home color as a safe fallback.
	static int firstDifferentAvailableShirtColorFromShuffledList(
		const std::array<int, kApprovedShirtColorCount>& randomizedShirtColors,
		const std::string& shirtBasePath,
		int homeShirtColor)
	{
		for (int shirtColor : randomizedShirtColors) {
			if (shirtColor == homeShirtColor) continue;
			if (!uniformFolderExistsForAllDirections(shirtBasePath, shirtColor)) continue;
			return shirtColor;
		}

		return homeShirtColor;
	}

	// Picks an Away shirt that exists, differs from Home, and is not white.
	// This keeps the usual soccer contrast: Home may wear white, Away should not.
	static int firstValidAwayShirtColorFromShuffledList(
		const std::array<int, kApprovedShirtColorCount>& randomizedShirtColors,
		const std::string& shirtBasePath,
		int homeShirtColor)
	{
		for (int shirtColor : randomizedShirtColors) {
			if (shirtColor == homeShirtColor) continue;
			if (shirtColor == kWhiteOutfieldShirtColor) continue;
			if (!uniformFolderExistsForAllDirections(shirtBasePath, shirtColor)) continue;
			return shirtColor;
		}

		return firstDifferentAvailableShirtColorFromShuffledList(
			randomizedShirtColors,
			shirtBasePath,
			homeShirtColor);
	}

	// Chooses randomized Home/Away shirt colors for outfield players.
	static std::array<int, 2> randomizedTeamShirtColors() {
		const std::string shirtBasePath = "assets/textures/Penzilla Protagonist/ShirtSheets/";
		const std::array<int, kApprovedShirtColorCount> randomizedShirtColors = shuffledApprovedShirtColors();
		const int homeShirtColor = firstAvailableShirtColorFromShuffledList(
			randomizedShirtColors,
			shirtBasePath);
		const int awayShirtColor = firstValidAwayShirtColorFromShuffledList(
			randomizedShirtColors,
			shirtBasePath,
			homeShirtColor);

		// If only one shirt color exists, both teams share it rather than
		// rendering missing art. With the current approved folders, this should
		// not happen because several shirt colors are present.
		return { homeShirtColor, awayShirtColor };
	}

	// Loads every direction for one team layer: either all shirts or all pants.
	static void loadUniformDirectionTextures(
		std::array<std::array<sf::Texture, 4>, 2>& teamTextures,
		const std::string& basePath,
		int teamIndex,
		int colorNumber)
	{
		for (int directionIndex = 0; directionIndex < kPlayerDirectionCount; ++directionIndex) {
			const bool loaded = loadUniformTexture(
				teamTextures[teamIndex][directionIndex],
				basePath,
				colorNumber,
				directionIndex);

			// Uniforms are art-only. If one file is missing, keep running
			// instead of crashing; the missing layer simply will not render.
			(void)loaded;
		}
	}

	// Loads the Home and Away colors for one full uniform layer.
	static void loadTeamLayerTextures(
		std::array<std::array<sf::Texture, 4>, 2>& teamTextures,
		const std::string& basePath,
		int homeColorNumber,
		int awayColorNumber)
	{
		loadUniformDirectionTextures(teamTextures, basePath, kHomeTeamIndex, homeColorNumber);
		loadUniformDirectionTextures(teamTextures, basePath, kAwayTeamIndex, awayColorNumber);
	}

	// Loads all four directions for a non-team-specific uniform layer.
	// Used by goalkeepers, because both keepers share the same black kit.
	static void loadSingleUniformLayerTextures(
		std::array<sf::Texture, 4>& uniformLayerTextures,
		const std::string& basePath,
		int colorNumber)
	{
		for (int directionIndex = 0; directionIndex < kPlayerDirectionCount; ++directionIndex) {
			const bool loaded = loadUniformTexture(
				uniformLayerTextures[directionIndex],
				basePath,
				colorNumber,
				directionIndex);

			// Goalkeeper art is best-effort like the outfield uniform layers.
			// If a file is missing, the game still runs and only that layer is blank.
			(void)loaded;
		}
	}

	// Loads randomized outfield shirts and fixed pants.
	// Home pants are white/light; Away pants are black/dark.
	static void loadTeamUniformTextures(
		std::array<std::array<sf::Texture, 4>, 2>& teamShirtTextures,
		std::array<std::array<sf::Texture, 4>, 2>& teamPantTextures)
	{
		const std::array<int, 2> teamShirtColors = randomizedTeamShirtColors();
		const int homeShirtColor = teamShirtColors[kHomeTeamIndex];
		const int awayShirtColor = teamShirtColors[kAwayTeamIndex];

		const std::string shirtBasePath = "assets/textures/Penzilla Protagonist/ShirtSheets/";
		const std::string pantBasePath = "assets/textures/Penzilla Protagonist/PantSheets/";

		// Shirts: Home and Away must be visually different.
		// Pants: soccer convention for this project is Home=white, Away=black.
		loadTeamLayerTextures(teamShirtTextures, shirtBasePath, homeShirtColor, awayShirtColor);
		loadTeamLayerTextures(teamPantTextures, pantBasePath, kHomeWhitePantsColor, kAwayBlackPantsColor);
	}

	// Loads the all-black goalkeeper kit.
	// Goalkeeper visuals do not depend on team so they stay easy to identify.
	static void loadGoalkeeperUniformTextures(
		std::array<sf::Texture, 4>& goalkeeperShirtTextures,
		std::array<sf::Texture, 4>& goalkeeperPantTextures)
	{
		const std::string shirtBasePath = "assets/textures/Penzilla Protagonist/ShirtSheets/";
		const std::string pantBasePath = "assets/textures/Penzilla Protagonist/PantSheets/";

		// Goalkeepers always wear the black kit. This is intentionally not
		// randomized so the keeper remains visually distinct every match.
		loadSingleUniformLayerTextures(goalkeeperShirtTextures, shirtBasePath, kGoalkeeperBlackUniformColor);
		loadSingleUniformLayerTextures(goalkeeperPantTextures, pantBasePath, kGoalkeeperBlackUniformColor);
	}

	// Small movement threshold for cow animation.
	static bool cowIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f; // 1 (world units/sec)^2 threshold; avoids flicker when nearly stopped
	}

	// Small movement threshold for loose-ball animation.
	static bool ballIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f;
	}

	// Checks if an owning player is moving while dribbling.
	static bool playerIsMoving(const PlayerState& playerState) {
		const float speedSq =
			playerState.velocity.x * playerState.velocity.x +
			playerState.velocity.y * playerState.velocity.y;

		return speedSq > 1.0f;
	}

	// Animates the ball while attached to a moving owner.
	static bool ownedBallShouldAnimate(const GameStatePacket& gameState) {
		if (gameState.ballOwnerPlayerId < 0) return false;

		const std::size_t ownerIndex = static_cast<std::size_t>(gameState.ballOwnerPlayerId);
		if (ownerIndex >= gameState.players.size()) return false;

		// When a player owns the ball, World usually attaches the ball to the
		// player and the ball velocity can be zero. Visually, the ball should
		// still animate if the owner is moving/dribbling.
		return playerIsMoving(gameState.players[ownerIndex]);
	}

	// Final decision for ball animation: loose moving ball OR dribbling owner.
	static bool ballSpriteShouldAnimate(const GameStatePacket& gameState) {
		if (ballIsMoving(gameState.ballVelocity)) return true;
		return ownedBallShouldAnimate(gameState);
	}

	// Converts a player facing vector into the direction index used by all
	// Penzilla player sprite layers.
	static int playerDirectionIndexFromFacingDirection(const sf::Vector2f& facingDirection) {
		if (std::abs(facingDirection.y) > std::abs(facingDirection.x)) {
			if (facingDirection.y > 0.f) return 0; // Down
			return 1; // Up
		}

		if (facingDirection.x > 0.f) return 3; // Right
		return 2; // Left
	}

	// Chooses the cow walking row from its velocity vector.
	static int cowDirectionRowFromVelocity(const sf::Vector2f& velocity) {
		// Row order (per your sheet):
		// 0 = Right, 1 = Left, 2 = Down, 3 = Up
		if (std::abs(velocity.x) >= std::abs(velocity.y)) {
			return (velocity.x >= 0.f) ? 0 : 1;
		}
		return (velocity.y >= 0.f) ? 2 : 3;
	}

	static int cowFrameFromTime(float timeSeconds) {
		// Converts time to an integer frame index in [0..7].
		// Small per-cow offsets are applied at the call site to avoid perfect sync.
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kCowWalkSecondsPerFrame);
		return frame % kCowWalkSheetColumns;
	}

	static int cowIdleFrameFromTime(float timeSeconds) {
		// Converts match time to a valid idle-cow frame index.
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kCowIdleSecondsPerFrame);
		return frame % kCowIdleFrameCount;
	}

	static sf::IntRect cowIdleTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		// Maps a flat idle-frame number into row/column coordinates.
		const int clampedFrame = std::clamp(frameIndex, 0, kCowIdleFrameCount - 1);
		const int row = clampedFrame / kCowIdleSheetColumns;
		const int column = clampedFrame % kCowIdleSheetColumns;
		return sf::IntRect({ column * frameWidth, row * frameHeight }, { frameWidth, frameHeight });
	}

	static int ballFrameFromTime(float timeSeconds) {
		// Converts match time to a valid animated-ball frame index.
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kBallSecondsPerFrame);
		return frame % kBallIconFrameCount;
	}

	static sf::IntRect ballTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		// Ball frames are in one row of Ball_Icons.png, starting at column 2
		// in human counting, so kBallIconFirstColumn is zero-based column 1.
		const int clampedFrame = std::clamp(frameIndex, 0, kBallIconFrameCount - 1);
		const int column = kBallIconFirstColumn + clampedFrame;
		return sf::IntRect({ column * frameWidth, kBallIconRow * frameHeight }, { frameWidth, frameHeight });
	}

	static sf::IntRect controlledPlayerIndicatorTextureRect(int frameWidth, int frameHeight) {
		// You identified Ball_Icons.png row 2, column 1 as a good "controlled
		// player" marker. Human counting says row 2/column 1; zero-based sheet
		// coordinates are row 1/column 0.
		return sf::IntRect(
			{ kControlledPlayerIndicatorColumn * frameWidth, kBallIconRow * frameHeight },
			{ frameWidth, frameHeight });
	}

	static bool validControlledPlayerId(std::uint8_t playerId, const GameStatePacket& gameState) {
		// Packet IDs are uint8_t, so this guard only needs the upper bound.
		return static_cast<std::size_t>(playerId) < gameState.players.size();
	}

	static sf::Vector2f controlledIndicatorPositionForPlayer(const PlayerState& playerState) {
		// Put the indicator above the player's head, not on top of the body.
		// The offset uses gameplay constants so it stays aligned if player size
		// changes later.
		return sf::Vector2f(
			playerState.position.x,
			playerState.position.y - Config::PLAYER_SIZE - 2.f);
	}

	static bool drawControlledPlayerIndicator(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalIndicatorSprite,
		const sf::Texture& ballIconTexture,
		const GameStatePacket& gameState,
		std::uint8_t playerId,
		sf::Color tintColor)
	{
		// Renderer stays snapshot-only: it uses controlledHomePlayerId and
		// controlledAwayPlayerId already present in GameStatePacket. This means
		// networking does not need any new fields for the indicator.
		if (!optionalIndicatorSprite) return false;
		if (!validControlledPlayerId(playerId, gameState)) return false;

		const sf::Vector2u sheetSize = ballIconTexture.getSize();
		const int frameWidth = static_cast<int>(sheetSize.x) / kBallIconSheetColumns;
		const int frameHeight = static_cast<int>(sheetSize.y) / kBallIconSheetRows;
		if (frameWidth <= 0) return false;
		if (frameHeight <= 0) return false;

		const PlayerState& controlledPlayer = gameState.players[playerId];
		const sf::Vector2f indicatorPosition = controlledIndicatorPositionForPlayer(controlledPlayer);

		optionalIndicatorSprite->setTextureRect(controlledPlayerIndicatorTextureRect(frameWidth, frameHeight));
		optionalIndicatorSprite->setOrigin({ frameWidth * 0.5f, frameHeight * 0.5f });

		// Slightly larger than the ball so it reads as a UI marker, not a second
		// ball. The texture is tinted by team so both controlled players can be
		// shown without ambiguity in host/client demos.
		const float desiredIndicatorDiameter = Config::PLAYER_SIZE * 0.85f;
		const float scaleX = desiredIndicatorDiameter / static_cast<float>(frameWidth);
		const float scaleY = desiredIndicatorDiameter / static_cast<float>(frameHeight);

		optionalIndicatorSprite->setScale({ scaleX, scaleY });
		optionalIndicatorSprite->setColor(tintColor);
		optionalIndicatorSprite->setPosition(indicatorPosition);
		window.draw(*optionalIndicatorSprite);
		return true;
	}

	static void drawControlledPlayerIndicators(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalIndicatorSprite,
		const sf::Texture& ballIconTexture,
		const GameStatePacket& gameState,
		bool showAwayControlledIndicator)
	{
		// Draw both controlled slots because Renderer intentionally does not know
		// whether it is running on host, client, or single-player. Host controls
		// Home and client controls Away, so the packet already identifies both.
		// Single-player can explicitly hide the Away marker because the away
		// team is fully AI-controlled there.
		const sf::Color homeIndicatorColor(80, 180, 255, 235);
		const sf::Color awayIndicatorColor(255, 210, 70, 235);

		(void)drawControlledPlayerIndicator(
			window,
			optionalIndicatorSprite,
			ballIconTexture,
			gameState,
			gameState.controlledHomePlayerId,
			homeIndicatorColor);

		if (!showAwayControlledIndicator) return;
		if (gameState.controlledAwayPlayerId == gameState.controlledHomePlayerId) return;

		(void)drawControlledPlayerIndicator(
			window,
			optionalIndicatorSprite,
			ballIconTexture,
			gameState,
			gameState.controlledAwayPlayerId,
			awayIndicatorColor);
	}

	static void drawFallbackBall(sf::RenderWindow& window, const sf::Vector2f& ballPosition) {
		// Shape fallback keeps the game playable if Ball_Icons.png is missing.
		sf::CircleShape ball(Config::BALL_RADIUS);
		ball.setOrigin({ Config::BALL_RADIUS, Config::BALL_RADIUS });
		ball.setPosition(ballPosition);
		ball.setFillColor(sf::Color::White);
		window.draw(ball);
	}

	static bool drawAnimatedBallSprite(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalBallSprite,
		const sf::Texture& ballTexture,
		const GameStatePacket& gameState)
	{
		// Draws the ball from Ball_Icons.png. Returns false when the sprite or
		// sheet dimensions are invalid so the caller can draw a fallback circle.
		if (!optionalBallSprite) return false;

		const sf::Vector2u ballSheetSize = ballTexture.getSize();
		const int frameWidth = static_cast<int>(ballSheetSize.x) / kBallIconSheetColumns;
		const int frameHeight = static_cast<int>(ballSheetSize.y) / kBallIconSheetRows;
		if (frameWidth <= 0) return false;
		if (frameHeight <= 0) return false;

		const int frameIndex = ballSpriteShouldAnimate(gameState)
			? ballFrameFromTime(gameState.matchTimerSec)
			: 0;

		optionalBallSprite->setTextureRect(ballTextureRectFromFrame(frameIndex, frameWidth, frameHeight));
		optionalBallSprite->setOrigin({ frameWidth * 0.5f, frameHeight * 0.5f });

		const float desiredDiameter = Config::BALL_RADIUS * 2.f;
		const float scaleX = desiredDiameter / static_cast<float>(frameWidth);
		const float scaleY = desiredDiameter / static_cast<float>(frameHeight);

		optionalBallSprite->setScale({ scaleX, scaleY });
		optionalBallSprite->setPosition(gameState.ballPosition);
		window.draw(*optionalBallSprite);
		return true;
	}

	static void drawFallbackCow(sf::RenderWindow& window, const sf::Vector2f& cowPosition) {
		// Shape fallback keeps cows visible even if a cow texture is missing.
		sf::CircleShape cowShape(Config::COW_RADIUS);
		cowShape.setOrigin({ Config::COW_RADIUS, Config::COW_RADIUS });
		cowShape.setPosition(cowPosition);
		cowShape.setFillColor(sf::Color(110, 75, 35));
		window.draw(cowShape);
	}

	static bool drawIdleCowSprite(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalCowIdleSprite,
		const sf::Texture& cowIdleTexture,
		const CowState& cow,
		float matchTimerSec,
		std::size_t cowSlotIndex)
	{
		// Draws a paused cow from the idle animation sheet.
		// cowSlotIndex offsets timing so all cows do not animate in sync.
		if (!optionalCowIdleSprite) return false;

		const sf::Vector2u idleSheetSize = cowIdleTexture.getSize();
		const int frameWidth = static_cast<int>(idleSheetSize.x) / kCowIdleSheetColumns;
		const int frameHeight = static_cast<int>(idleSheetSize.y) / kCowIdleSheetRows;
		if (frameWidth <= 0) return false;
		if (frameHeight <= 0) return false;

		const float animationTime = matchTimerSec + (static_cast<float>(cowSlotIndex) * 0.23f);
		const int frameIndex = cowIdleFrameFromTime(animationTime);

		optionalCowIdleSprite->setTextureRect(cowIdleTextureRectFromFrame(frameIndex, frameWidth, frameHeight));
		optionalCowIdleSprite->setOrigin({ frameWidth * 0.5f, frameHeight * 0.5f });

		const float desiredDiameter = Config::COW_RADIUS * 2.f;
		const float scaleX = desiredDiameter / static_cast<float>(frameWidth);
		const float scaleY = desiredDiameter / static_cast<float>(frameHeight);

		optionalCowIdleSprite->setScale({ scaleX, scaleY });
		optionalCowIdleSprite->setPosition(cow.position);
		window.draw(*optionalCowIdleSprite);
		return true;
	}

	static bool drawWalkingCowSprite(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalCowSprite,
		const sf::Texture& cowTexture,
		const CowState& cow,
		float matchTimerSec,
		std::size_t cowSlotIndex)
	{
		// Draws a moving cow from the directional walking sheet.
		// Returns false if the texture is unavailable so a fallback can draw.
		if (!optionalCowSprite) return false;

		const sf::Vector2u walkSheetSize = cowTexture.getSize();
		const int frameWidth = static_cast<int>(walkSheetSize.x) / kCowWalkSheetColumns;
		const int frameHeight = static_cast<int>(walkSheetSize.y) / kCowWalkSheetRows;
		if (frameWidth <= 0) return false;
		if (frameHeight <= 0) return false;

		const bool moving = cowIsMoving(cow.velocity);
		const int row = moving ? cowDirectionRowFromVelocity(cow.velocity) : 2;

		const float animationTime = matchTimerSec + (static_cast<float>(cowSlotIndex) * 0.17f);
		const int column = moving ? cowFrameFromTime(animationTime) : 0;

		optionalCowSprite->setTextureRect(
			sf::IntRect({ column * frameWidth, row * frameHeight }, { frameWidth, frameHeight }));
		optionalCowSprite->setOrigin({ frameWidth * 0.5f, frameHeight * 0.5f });

		const float desiredDiameter = Config::COW_RADIUS * 2.f;
		const float scaleX = desiredDiameter / static_cast<float>(frameWidth);
		const float scaleY = desiredDiameter / static_cast<float>(frameHeight);

		optionalCowSprite->setScale({ scaleX, scaleY });
		optionalCowSprite->setPosition(cow.position);
		window.draw(*optionalCowSprite);
		return true;
	}

	static void drawCowFromSnapshot(
		sf::RenderWindow& window,
		std::optional<sf::Sprite>& optionalCowWalkingSprite,
		const sf::Texture& cowWalkingTexture,
		std::optional<sf::Sprite>& optionalCowIdleSprite,
		const sf::Texture& cowIdleTexture,
		const CowState& cowState,
		float matchTimerSec,
		std::size_t cowSlotIndex)
	{
		// Chooses the correct cow visual for one cow snapshot.
		// Idle cows use the idle sheet; moving cows use the directional walk sheet.
		// If either texture is missing, the fallback circle still shows the cow.
		const bool shouldUseIdleAnimation = !cowIsMoving(cowState.velocity);

		if (shouldUseIdleAnimation) {
			const bool drewIdleCow = drawIdleCowSprite(
				window,
				optionalCowIdleSprite,
				cowIdleTexture,
				cowState,
				matchTimerSec,
				cowSlotIndex);
			if (drewIdleCow) return;
		}

		const bool drewWalkingCow = drawWalkingCowSprite(
			window,
			optionalCowWalkingSprite,
			cowWalkingTexture,
			cowState,
			matchTimerSec,
			cowSlotIndex);
		if (drewWalkingCow) return;

		drawFallbackCow(window, cowState.position);
	}

	static void drawPlayerSpriteLayer(
		sf::RenderWindow& window,
		sf::Sprite& reusablePlayerSprite,
		const sf::Texture& layerTexture,
		int currentFrame,
		sf::Vector2f playerPosition,
		float playerScale,
		int frameWidth,
		int frameHeight)
	{
		// Player art is built from separate sheets: body, pants, then shirt.
		// Each call fully configures the reusable sprite so the layers do not
		// depend on whatever state a previous layer left behind.
		reusablePlayerSprite.setTexture(layerTexture);
		reusablePlayerSprite.setTextureRect(sf::IntRect(
			{ currentFrame * frameWidth, 0 },
			{ frameWidth, frameHeight }));
		reusablePlayerSprite.setPosition(playerPosition);
		reusablePlayerSprite.setScale({ playerScale, playerScale });

		window.draw(reusablePlayerSprite);
	}

	static const sf::Texture& pantsTextureForPlayer(
		const PlayerState& playerState,
		int playerDirectionIndex,
		int teamIndex,
		const std::array<sf::Texture, 4>& goalkeeperPantTextures,
		const std::array<std::array<sf::Texture, 4>, 2>& teamPantTextures)
	{
		// Goalkeepers always override team pants with the all-black goalkeeper kit.
		if (playerState.isGoalkeeper) return goalkeeperPantTextures[playerDirectionIndex];

		return teamPantTextures[teamIndex][playerDirectionIndex];
	}

	static const sf::Texture& shirtTextureForPlayer(
		const PlayerState& playerState,
		int playerDirectionIndex,
		int teamIndex,
		const std::array<sf::Texture, 4>& goalkeeperShirtTextures,
		const std::array<std::array<sf::Texture, 4>, 2>& teamShirtTextures)
	{
		// Goalkeepers always override team shirts with the all-black goalkeeper kit.
		if (playerState.isGoalkeeper) return goalkeeperShirtTextures[playerDirectionIndex];

		return teamShirtTextures[teamIndex][playerDirectionIndex];
	}
}

Renderer::Renderer() {
	// 1. Initialize Background
	// Uses the repo convention "assets/backgrounds/".
	if (mBackgroundTex.loadFromFile("assets/backgrounds/footballPitchSponsors.png")) {
		mBackgroundSprite.emplace(mBackgroundTex);
		float scaleX = static_cast<float>(Config::WINDOW_WIDTH) / mBackgroundTex.getSize().x;
		float scaleY = static_cast<float>(Config::WINDOW_HEIGHT) / mBackgroundTex.getSize().y;
		mBackgroundSprite->setScale({ scaleX, scaleY });
	}

	// 2. Load player skin sheets.
	// The old renderer selected one CharacterSheets folder per team, which made
	// every player on that team share the same skin color. Now each player ID
	// receives one randomized realistic skin folder from Color2 through Color10.
	loadPlayerSkinTextures(mPlayerSkinTextures);

	// Uniforms are team-level: every Home player shares one shirt+pants combo,
	// and every Away player shares a different combo. The body/skin remains
	// player-level so the team does not all have the same skin tone.
	loadTeamUniformTextures(mTeamShirtTextures, mTeamPantTextures);

	// Goalkeepers use a separate all-black kit, so they never inherit randomized
	// outfield shirts or team pants.
	loadGoalkeeperUniformTextures(mGoalkeeperShirtTextures, mGoalkeeperPantTextures);

	// 3. Emplace Player Sprite using the first player/direction texture.
	mPlayerSprite.emplace(mPlayerSkinTextures[0][0]);
	mPlayerSprite->setOrigin({ 12.f, 12.f });

	// Load cow sprite (optional). If this fails, we'll draw cows as circles.
	if (mCowTexture.loadFromFile(Config::COW_TEXTURE_PATH)) {
		mCowSprite.emplace(mCowTexture);

		const auto sz = mCowTexture.getSize();
		mCowSprite->setOrigin({
			static_cast<float>(sz.x) * 0.5f,
			static_cast<float>(sz.y) * 0.5f
			});
	}

	// Idle cow animation sheet: 13 columns x 3 rows, with 36 real frames.
	if (mCowIdleTexture.loadFromFile("assets/textures/cow idle animation.png")) {
		mCowIdleSprite.emplace(mCowIdleTexture);
	}

	// Ball_Icons.png is a small icon sheet. The ball frames are on row 2,
	// columns 2-4 (using human 1-based counting). Row 2, column 1 is reused
	// as the controlled-player indicator.
	if (mBallTexture.loadFromFile("assets/textures/Ball_Icons.png")) {
		mBallSprite.emplace(mBallTexture);
		mControlledPlayerIndicatorSprite.emplace(mBallTexture);
	}

	// Load game-over overlay assets. These are presentation-only and are drawn
	// from the authoritative snapshot state; they do not change match rules.
	if (mGameOverPanelTex.loadFromFile("assets/UI/sfml_ui_assets/background_settings.png")) {
		mGameOverPanelSprite.emplace(mGameOverPanelTex);

		const float scaleX = static_cast<float>(Config::WINDOW_WIDTH) / mGameOverPanelTex.getSize().x;
		const float scaleY = static_cast<float>(Config::WINDOW_HEIGHT) / mGameOverPanelTex.getSize().y;

		mGameOverPanelSprite->setScale({ scaleX, scaleY });
		mGameOverPanelSprite->setPosition({ 0.f, 0.f });
	}

	if (mGameOverTextTex.loadFromFile("assets/UI/GameOver.png")) {
		mGameOverTextSprite.emplace(mGameOverTextTex);

		mGameOverTextSprite->setScale({ 0.42f, 0.42f });

		mGameOverTextSprite->setPosition({
			Config::WINDOW_WIDTH / 2.f - (mGameOverTextTex.getSize().x * 0.42f) / 2.f,
			30.f
			});
	}

	if (mTryAgainTex.loadFromFile("assets/UI/TryAgain.png")) {
		mTryAgainSprite.emplace(mTryAgainTex);

		mTryAgainSprite->setScale({ 0.42f, 0.42f });
		mTryAgainSprite->setPosition({
			Config::WINDOW_WIDTH / 2.f - (mTryAgainTex.getSize().x * 0.42f) / 2.f,
			340.f
			});
	}

	if (mQuitTex.loadFromFile("assets/UI/QuitB.png")) {
		mQuitSprite.emplace(mQuitTex);

		mQuitSprite->setScale({ 0.15f, 0.15f });
		mQuitSprite->setPosition({
			Config::WINDOW_WIDTH / 2.f - (mQuitTex.getSize().x * 0.15f) / 2.f,
			380.f
			});
	}

}

void Renderer::render(
	sf::RenderWindow& window,
	const GameStatePacket& gameState,
	bool showAwayControlledIndicator,
	bool showGameOverOverlay)
{
	// A. Draw Background
	window.setView(window.getDefaultView());
	if (mBackgroundSprite) window.draw(*mBackgroundSprite);

	// B. Switch to World View
	sf::View worldView(gameState.pitchBounds);
	worldView.setViewport(sf::FloatRect({ 0.f, 0.f }, { 1.f, 1.f }));
	window.setView(worldView);

	// 1. Draw Ball
	const bool drewBallSprite = drawAnimatedBallSprite(window, mBallSprite, mBallTexture, gameState);
	if (!drewBallSprite) drawFallbackBall(window, gameState.ballPosition);

	// 1.5 Draw Cows
	// Cows are host-simulated obstacles. Client renders from packet only.
	for (std::size_t cowSlotIndex = 0; cowSlotIndex < gameState.cows.size(); ++cowSlotIndex) {
		const CowState& cowState = gameState.cows[cowSlotIndex];
		if (!cowState.active) continue;

		drawCowFromSnapshot(
			window,
			mCowSprite,
			mCowTexture,
			mCowIdleSprite,
			mCowIdleTexture,
			cowState,
			gameState.matchTimerSec,
			cowSlotIndex);
	}

	// 2. Draw Animated Players
	int currentFrame = static_cast<int>(gameState.matchTimerSec / ANIM_SPEED) % 4;

	for (std::size_t playerIndex = 0; playerIndex < gameState.players.size(); ++playerIndex) {
		const PlayerState& playerState = gameState.players[playerIndex];

		const int playerDirectionIndex = playerDirectionIndexFromFacingDirection(playerState.facingDirection);
		const int teamIndex = safeTeamIndexFromPlayerState(playerState, playerIndex);
		const sf::Texture& pantsTexture = pantsTextureForPlayer(
			playerState,
			playerDirectionIndex,
			teamIndex,
			mGoalkeeperPantTextures,
			mTeamPantTextures);
		const sf::Texture& shirtTexture = shirtTextureForPlayer(
			playerState,
			playerDirectionIndex,
			teamIndex,
			mGoalkeeperShirtTextures,
			mTeamShirtTextures);

		// Scale sprite to match gameplay half-size constants.
		const float playerSpriteScale = (Config::PLAYER_HALF_SIZE * 2.2f) / FRAME_WIDTH;

		// Player sprites are layered:
		// 1. body/skin selected per player
		// 2. pants selected per team
		// 3. shirt selected per team
		// This gives realistic skin variation while preserving clear uniforms.
		drawPlayerSpriteLayer(
			window,
			*mPlayerSprite,
			mPlayerSkinTextures[playerIndex][playerDirectionIndex],
			currentFrame,
			playerState.position,
			playerSpriteScale,
			FRAME_WIDTH,
			FRAME_HEIGHT);

		drawPlayerSpriteLayer(
			window,
			*mPlayerSprite,
			pantsTexture,
			currentFrame,
			playerState.position,
			playerSpriteScale,
			FRAME_WIDTH,
			FRAME_HEIGHT);

		drawPlayerSpriteLayer(
			window,
			*mPlayerSprite,
			shirtTexture,
			currentFrame,
			playerState.position,
			playerSpriteScale,
			FRAME_WIDTH,
			FRAME_HEIGHT);
	}

	// 3. Draw controlled-player indicators above the player sprites.
	// These are UI markers only. They come from Ball_Icons.png and use the
	// controlled player IDs already carried by GameStatePacket.
	drawControlledPlayerIndicators(
		window,
		mControlledPlayerIndicatorSprite,
		mBallTexture,
		gameState,
		showAwayControlledIndicator);

	// C. Draw HUD
	window.setView(window.getDefaultView());
	renderHUD(window, (int)gameState.scoreHome, (int)gameState.scoreAway, gameState.matchTimerSec, (int)gameState.currentState);

	if (showGameOverOverlay && gameState.currentState == 3) {
		if (mGameOverPanelSprite) window.draw(*mGameOverPanelSprite);
		if (mGameOverTextSprite) window.draw(*mGameOverTextSprite);

		const sf::Font* font = getHudFont();
		if (font) {
			drawCenteredHudText(
				window,
				*font,
				winnerTextFromGameState(gameState),
				Config::WINDOW_WIDTH * 0.5f,
				240.f,
				sf::Color(255, 230, 120),
				34);

			drawCenteredHudText(
				window,
				*font,
				finalScoreTextFromGameState(gameState),
				Config::WINDOW_WIDTH * 0.5f,
				285.f,
				sf::Color::White,
				24);
		}

		if (mTryAgainSprite) window.draw(*mTryAgainSprite);
		if (mQuitSprite) window.draw(*mQuitSprite);
	}
}

void Renderer::renderHUD(
	sf::RenderWindow& window,
	int homeScore,
	int awayScore,
	float matchTimerSec,
	int stateId)
{
	const sf::Font* font = getHudFont();
	if (!font) return;

	drawHudText(
		window,
		*font,
		"HOME " + std::to_string(homeScore) + " - " + std::to_string(awayScore) + " AWAY",
		{ 20.f, 20.f },
		sf::Color::White,
		24);

	drawHudText(
		window,
		*font,
		"TIME: " + formatClock(matchTimerSec),
		{ 20.f, 50.f },
		sf::Color::White,
		24);

	drawHudText(
		window,
		*font,
		stateNameFromId(stateId),
		{ (float)Config::WINDOW_WIDTH / 2.f - 50.f, 20.f },
		sf::Color::Yellow,
		30);
}
