#include "Renderer.h"

/**
 * @file Renderer.cpp
 * @brief Snapshot-only rendering implementation.
 *
 * AI disclosure:
 * The layered player sprite rendering, randomized visual-only uniforms, cow
 * sprite animation, ball animation, and fallback drawing were generated/revised
 * with help from OpenAI Codex.
 *
 * Prompt used:
 * "Help me implement an SFML renderer that consumes GameStatePacket only. Layer
 * body, pants, and shirt sprites; randomize visual kits without changing
 * networking; animate cows and ball; and keep the code readable."
 */

#include "Common/Constants.h"
#include <algorithm>
#include <chrono>
#include <cmath> // SANTI 30/04/26: cow animation direction selection
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>

namespace {
	// SANTI 30/04/26
	// Loads the HUD font once and reuses it for every frame.
	// Returns nullptr if the font is missing so HUD rendering can fail safely.
	const sf::Font* getHudFont() {
		static sf::Font font;
		static bool loaded = font.openFromFile("assets/fonts/arial.ttf");
		return loaded ? &font : nullptr;
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
	// Formats the match clock as M:SS. The timer counts up in GameStatePacket.
	std::string formatClock(float seconds) {
		int total = std::max(0, static_cast<int>(seconds));
		std::ostringstream oss;
		oss << (total / 60) << ":" << std::setfill('0') << std::setw(2) << (total % 60);
		return oss.str();
	}

	// SANTI 30/04/26: HUD text helper (no lambdas).
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

	// ------------------------------------------------------------------------
	// COW AND BALL SPRITESHEET ANIMATION (SANTI: COWS 30/04/26)
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

	// SANTI 30/04/26
	// These are the outfield shirt folders still approved in assets.
	// Color3 is intentionally excluded because goalkeepers own the all-black kit.
	// This prevents an outfield team from randomly wearing black shirts with
	// black pants and looking like goalkeepers.
	static constexpr std::array<int, kApprovedShirtColorCount> kApprovedShirtColors = {
		1, 2, 4, 5, 6, 7, 9, 10
	};

	// SANTI 30/04/26
	// Creates a local renderer RNG for visual-only randomization.
	// This is intentionally not part of GameStatePacket because shirt/skin
	// randomization does not affect gameplay or host-authoritative simulation.
	static std::mt19937 makeRendererRandomGenerator() {
		const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		const unsigned int timeSeed = static_cast<unsigned int>(now);
		const unsigned int deviceSeed = std::random_device{}();
		return std::mt19937(timeSeed ^ (deviceSeed + 0x9e3779b9u));
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
	// Selects one randomized skin-folder number for each player slot.
	// Player IDs keep stable skin colors for the lifetime of this Renderer.
	static std::array<int, Config::kNumPlayers> randomizedPlayerSkinColors() {
		std::array<int, kPlayerSkinColorCount> availableSkinColors{};

		for (int i = 0; i < kPlayerSkinColorCount; ++i) {
			availableSkinColors[i] = kFirstRealisticSkinColor + i;
		}

		// SANTI 30/04/26
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

	// SANTI 30/04/26
	// Converts Color2, Color3, etc. folder numbers into folder names.
	static std::string skinFolderNameFromColorNumber(int skinColorNumber) {
		return "Color" + std::to_string(skinColorNumber);
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
	// Uses packet teamId when valid, and falls back to the player ID contract
	// when a packet has an unexpected teamId.
	static int safeTeamIndexFromPlayerState(const PlayerState& playerState, std::size_t playerIndex) {
		if (playerState.teamId < kTeamCount) return static_cast<int>(playerState.teamId);
		return (playerIndex < 4) ? kHomeTeamIndex : kAwayTeamIndex;
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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

				// SANTI 30/04/26
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

	// SANTI 30/04/26
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

	// SANTI 30/04/26
	// Verifies that a ColorN uniform folder has all four direction files before
	// it is selected for a team uniform.
	static bool uniformFolderExistsForAllDirections(const std::string& basePath, int colorNumber) {
		for (int directionIndex = 0; directionIndex < kPlayerDirectionCount; ++directionIndex) {
			const std::string fullPath = spriteSheetPath(basePath, colorNumber, directionIndex);

			if (!std::filesystem::exists(fullPath)) return false;
		}

		return true;
	}

	// SANTI 30/04/26
	// Returns approved outfield shirt colors in randomized order.
	// Color3 is excluded globally because it belongs to goalkeepers.
	static std::array<int, kApprovedShirtColorCount> shuffledApprovedShirtColors() {
		std::array<int, kApprovedShirtColorCount> randomizedShirtColors = kApprovedShirtColors;
		std::mt19937 randomGenerator = makeRendererRandomGenerator();
		std::shuffle(randomizedShirtColors.begin(), randomizedShirtColors.end(), randomGenerator);
		return randomizedShirtColors;
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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

		// SANTI 30/04/26
		// If only one shirt color exists, both teams share it rather than
		// rendering missing art. With the current approved folders, this should
		// not happen because several shirt colors are present.
		return { homeShirtColor, awayShirtColor };
	}

	// SANTI 30/04/26
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

			// SANTI 30/04/26
			// Uniforms are art-only. If one file is missing, keep running
			// instead of crashing; the missing layer simply will not render.
			(void)loaded;
		}
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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

			// SANTI 30/04/26
			// Goalkeeper art is best-effort like the outfield uniform layers.
			// If a file is missing, the game still runs and only that layer is blank.
			(void)loaded;
		}
	}

	// SANTI 30/04/26
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

		// SANTI 30/04/26
		// Shirts: Home and Away must be visually different.
		// Pants: soccer convention for this project is Home=white, Away=black.
		loadTeamLayerTextures(teamShirtTextures, shirtBasePath, homeShirtColor, awayShirtColor);
		loadTeamLayerTextures(teamPantTextures, pantBasePath, kHomeWhitePantsColor, kAwayBlackPantsColor);
	}

	// SANTI 30/04/26
	// Loads the all-black goalkeeper kit.
	// Goalkeeper visuals do not depend on team so they stay easy to identify.
	static void loadGoalkeeperUniformTextures(
		std::array<sf::Texture, 4>& goalkeeperShirtTextures,
		std::array<sf::Texture, 4>& goalkeeperPantTextures)
	{
		const std::string shirtBasePath = "assets/textures/Penzilla Protagonist/ShirtSheets/";
		const std::string pantBasePath = "assets/textures/Penzilla Protagonist/PantSheets/";

		// SANTI 30/04/26
		// Goalkeepers always wear the black kit. This is intentionally not
		// randomized so the keeper remains visually distinct every match.
		loadSingleUniformLayerTextures(goalkeeperShirtTextures, shirtBasePath, kGoalkeeperBlackUniformColor);
		loadSingleUniformLayerTextures(goalkeeperPantTextures, pantBasePath, kGoalkeeperBlackUniformColor);
	}

	// SANTI 30/04/26
	// Small movement threshold for cow animation.
	static bool cowIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f; // 1 (world units/sec)^2 threshold; avoids flicker when nearly stopped
	}

	// SANTI 30/04/26
	// Small movement threshold for loose-ball animation.
	static bool ballIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f;
	}

	// SANTI 30/04/26
	// Checks if an owning player is moving while dribbling.
	static bool playerIsMoving(const PlayerState& playerState) {
		const float speedSq =
			playerState.velocity.x * playerState.velocity.x +
			playerState.velocity.y * playerState.velocity.y;

		return speedSq > 1.0f;
	}

	// SANTI 30/04/26
	// Animates the ball while attached to a moving owner.
	static bool ownedBallShouldAnimate(const GameStatePacket& gameState) {
		if (gameState.ballOwnerPlayerId < 0) return false;

		const std::size_t ownerIndex = static_cast<std::size_t>(gameState.ballOwnerPlayerId);
		if (ownerIndex >= gameState.players.size()) return false;

		// SANTI 30/04/26
		// When a player owns the ball, World usually attaches the ball to the
		// player and the ball velocity can be zero. Visually, the ball should
		// still animate if the owner is moving/dribbling.
		return playerIsMoving(gameState.players[ownerIndex]);
	}

	// SANTI 30/04/26
	// Final decision for ball animation: loose moving ball OR dribbling owner.
	static bool ballSpriteShouldAnimate(const GameStatePacket& gameState) {
		if (ballIsMoving(gameState.ballVelocity)) return true;
		return ownedBallShouldAnimate(gameState);
	}

	// SANTI 30/04/26
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

	// SANTI 30/04/26
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
		// SANTI 30/04/26
		// Converts match time to a valid idle-cow frame index.
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kCowIdleSecondsPerFrame);
		return frame % kCowIdleFrameCount;
	}

	static sf::IntRect cowIdleTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		// SANTI 30/04/26
		// Maps a flat idle-frame number into row/column coordinates.
		const int clampedFrame = std::clamp(frameIndex, 0, kCowIdleFrameCount - 1);
		const int row = clampedFrame / kCowIdleSheetColumns;
		const int column = clampedFrame % kCowIdleSheetColumns;
		return sf::IntRect({ column * frameWidth, row * frameHeight }, { frameWidth, frameHeight });
	}

	static int ballFrameFromTime(float timeSeconds) {
		// SANTI 30/04/26
		// Converts match time to a valid animated-ball frame index.
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kBallSecondsPerFrame);
		return frame % kBallIconFrameCount;
	}

	static sf::IntRect ballTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		// SANTI 30/04/26
		// Ball frames are in one row of Ball_Icons.png, starting at column 2
		// in human counting, so kBallIconFirstColumn is zero-based column 1.
		const int clampedFrame = std::clamp(frameIndex, 0, kBallIconFrameCount - 1);
		const int column = kBallIconFirstColumn + clampedFrame;
		return sf::IntRect({ column * frameWidth, kBallIconRow * frameHeight }, { frameWidth, frameHeight });
	}

	static void drawFallbackBall(sf::RenderWindow& window, const sf::Vector2f& ballPosition) {
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
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
		// SANTI 30/04/26
		// Goalkeepers always override team shirts with the all-black goalkeeper kit.
		if (playerState.isGoalkeeper) return goalkeeperShirtTextures[playerDirectionIndex];

		return teamShirtTextures[teamIndex][playerDirectionIndex];
	}
}

Renderer::Renderer() {
	// 1. Initialize Background
	// SANTI 30/04/26: Use the shared repo convention "assets/backgrounds/".
	if (mBackgroundTex.loadFromFile("assets/backgrounds/footballPitch.png")) {
		mBackgroundSprite.emplace(mBackgroundTex);
		float scaleX = static_cast<float>(Config::WINDOW_WIDTH) / mBackgroundTex.getSize().x;
		float scaleY = static_cast<float>(Config::WINDOW_HEIGHT) / mBackgroundTex.getSize().y;
		mBackgroundSprite->setScale({ scaleX, scaleY });
	}

	// 2. Load player skin sheets.
	// SANTI 30/04/26
	// The old renderer selected one CharacterSheets folder per team, which made
	// every player on that team share the same skin color. Now each player ID
	// receives one randomized realistic skin folder from Color2 through Color10.
	loadPlayerSkinTextures(mPlayerSkinTextures);

	// SANTI 30/04/26
	// Uniforms are team-level: every Home player shares one shirt+pants combo,
	// and every Away player shares a different combo. The body/skin remains
	// player-level so the team does not all have the same skin tone.
	loadTeamUniformTextures(mTeamShirtTextures, mTeamPantTextures);

	// SANTI 30/04/26
	// Goalkeepers use a separate all-black kit, so they never inherit randomized
	// outfield shirts or team pants.
	loadGoalkeeperUniformTextures(mGoalkeeperShirtTextures, mGoalkeeperPantTextures);

	// 3. Emplace Player Sprite using the first player/direction texture.
	mPlayerSprite.emplace(mPlayerSkinTextures[0][0]);
	mPlayerSprite->setOrigin({ 12.f, 12.f });

	// SANTI: COWS 29/04/26
	// Load cow sprite (optional). If this fails, we'll draw cows as circles.
	if (mCowTexture.loadFromFile(Config::COW_TEXTURE_PATH)) {
		mCowSprite.emplace(mCowTexture);

		const auto sz = mCowTexture.getSize();
		mCowSprite->setOrigin({
			static_cast<float>(sz.x) * 0.5f,
			static_cast<float>(sz.y) * 0.5f
			});
	}

	// SANTI: COWS 30/04/26
	// Idle cow animation sheet: 13 columns x 3 rows, with 36 real frames.
	if (mCowIdleTexture.loadFromFile("assets/textures/cow idle animation.png")) {
		mCowIdleSprite.emplace(mCowIdleTexture);
	}

	// SANTI 30/04/26
	// Ball_Icons.png is a small icon sheet. The ball frames are on row 2,
	// columns 2-4 (using human 1-based counting).
	if (mBallTexture.loadFromFile("assets/textures/Ball_Icons.png")) {
		mBallSprite.emplace(mBallTexture);
	}
}

void Renderer::render(sf::RenderWindow& window, const GameStatePacket& gameState) {
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

	// 1.5 Draw Cows (SANTI: COWS 29/04/26)
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

		// SANTI 30/04/26
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

	// C. Draw HUD
	window.setView(window.getDefaultView());
	renderHUD(window, (int)gameState.scoreHome, (int)gameState.scoreAway, gameState.matchTimerSec, (int)gameState.currentState);
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
