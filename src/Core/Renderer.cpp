#include "Renderer.h"
#include "Common/Constants.h"
#include <algorithm>
#include <cmath> // SANTI 30/04/26: cow animation direction selection
#include <iomanip>
#include <sstream>

namespace {
	const sf::Font* getHudFont() {
		static sf::Font font;
		static bool loaded = font.openFromFile("assets/fonts/arial.ttf");
		return loaded ? &font : nullptr;
	}

	std::string stateNameFromId(int stateId) {
		switch (stateId) {
		case 0: return "Kickoff";
		case 1: return "Playing";
		case 2: return "Goal";
		case 3: return "GameOver";
		default: return "Unknown";
		}
	}

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

	static bool cowIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f; // 1 (world units/sec)^2 threshold; avoids flicker when nearly stopped
	}

	static bool ballIsMoving(const sf::Vector2f& velocity) {
		const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;
		return speedSq > 1.0f;
	}

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
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kCowIdleSecondsPerFrame);
		return frame % kCowIdleFrameCount;
	}

	static sf::IntRect cowIdleTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		const int clampedFrame = std::clamp(frameIndex, 0, kCowIdleFrameCount - 1);
		const int row = clampedFrame / kCowIdleSheetColumns;
		const int column = clampedFrame % kCowIdleSheetColumns;
		return sf::IntRect({ column * frameWidth, row * frameHeight }, { frameWidth, frameHeight });
	}

	static int ballFrameFromTime(float timeSeconds) {
		if (timeSeconds < 0.f) timeSeconds = 0.f;
		const int frame = static_cast<int>(timeSeconds / kBallSecondsPerFrame);
		return frame % kBallIconFrameCount;
	}

	static sf::IntRect ballTextureRectFromFrame(int frameIndex, int frameWidth, int frameHeight) {
		const int clampedFrame = std::clamp(frameIndex, 0, kBallIconFrameCount - 1);
		const int column = kBallIconFirstColumn + clampedFrame;
		return sf::IntRect({ column * frameWidth, kBallIconRow * frameHeight }, { frameWidth, frameHeight });
	}

	static void drawFallbackBall(sf::RenderWindow& window, const sf::Vector2f& ballPosition) {
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
		if (!optionalBallSprite) return false;

		const sf::Vector2u ballSheetSize = ballTexture.getSize();
		const int frameWidth = static_cast<int>(ballSheetSize.x) / kBallIconSheetColumns;
		const int frameHeight = static_cast<int>(ballSheetSize.y) / kBallIconSheetRows;
		if (frameWidth <= 0) return false;
		if (frameHeight <= 0) return false;

		const int frameIndex = ballIsMoving(gameState.ballVelocity)
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

	// 2. Load Teammate's Animated Sheets
	std::string paths[2] = { "Color1/", "Color6/" };
	std::string files[4] = { "Down.png", "Up.png", "Left.png", "Right.png" };
	std::string base = "assets/textures/Penzilla Protagonist/CharacterSheets/";

	for (int team = 0; team < 2; ++team) {
		for (int dir = 0; dir < 4; ++dir) {
			// SANTI 30/04/26: SFML marks loadFromFile as [[nodiscard]].
			// We attempt to load, but this is "best effort" (art can be missing in dev).
			const bool loaded = mTeamTextures[team][dir].loadFromFile(base + paths[team] + files[dir]);
			(void)loaded;
		}
	}

	// 3. Emplace Player Sprite using the first available texture
	mPlayerSprite.emplace(mTeamTextures[0][0]);
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
		const auto& cow = gameState.cows[cowSlotIndex];
		if (!cow.active) continue;

		const bool shouldUseIdleAnimation = !cowIsMoving(cow.velocity);
		if (shouldUseIdleAnimation) {
			const bool drewIdleCow = drawIdleCowSprite(
				window,
				mCowIdleSprite,
				mCowIdleTexture,
				cow,
				gameState.matchTimerSec,
				cowSlotIndex);
			if (drewIdleCow) continue;
		}

		const bool drewWalkingCow = drawWalkingCowSprite(
			window,
			mCowSprite,
			mCowTexture,
			cow,
			gameState.matchTimerSec,
			cowSlotIndex);
		if (drewWalkingCow) continue;

		// Fallback: simple circle if texture is missing.
		drawFallbackCow(window, cow.position);
	}

	// 2. Draw Animated Players
	int currentFrame = static_cast<int>(gameState.matchTimerSec / ANIM_SPEED) % 4;

	for (const auto& p : gameState.players) {
		int dir = 0; // Down
		if (std::abs(p.facingDirection.y) > std::abs(p.facingDirection.x)) {
			dir = (p.facingDirection.y > 0) ? 0 : 1; // Down/Up
		}
		else {
			dir = (p.facingDirection.x > 0) ? 3 : 2; // Right/Left
		}

		mPlayerSprite->setTexture(mTeamTextures[p.teamId][dir]);
		mPlayerSprite->setTextureRect(sf::IntRect({ currentFrame * FRAME_WIDTH, 0 }, { FRAME_WIDTH, FRAME_HEIGHT }));
		mPlayerSprite->setPosition(p.position);

		// Scale sprite to match gameplay half-size constants
		float scale = (Config::PLAYER_HALF_SIZE * 2.2f) / FRAME_WIDTH;
		mPlayerSprite->setScale({ scale, scale });

		window.draw(*mPlayerSprite);
	}

	// C. Draw HUD
	window.setView(window.getDefaultView());
	renderHUD(window, (int)gameState.scoreHome, (int)gameState.scoreAway, gameState.matchTimerSec, (int)gameState.currentState);
}

void Renderer::renderHUD(sf::RenderWindow& window, int hScore, int aScore, float time, int state) {
	const sf::Font* font = getHudFont();
	if (!font) return;

	drawHudText(
		window,
		*font,
		"HOME " + std::to_string(hScore) + " - " + std::to_string(aScore) + " AWAY",
		{ 20.f, 20.f },
		sf::Color::White,
		24);

	drawHudText(
		window,
		*font,
		"TIME: " + formatClock(time),
		{ 20.f, 50.f },
		sf::Color::White,
		24);

	drawHudText(
		window,
		*font,
		stateNameFromId(state),
		{ (float)Config::WINDOW_WIDTH / 2.f - 50.f, 20.f },
		sf::Color::Yellow,
		30);
}
