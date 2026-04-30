#include "Renderer.h"
#include "Common/Constants.h"
#include <algorithm>
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
}

Renderer::Renderer() {
	// 1. Initialize Background
	if (mBackgroundTex.loadFromFile("assets/background/football_field_without_holes.png")) {
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
	sf::CircleShape ball(Config::BALL_RADIUS);
	ball.setOrigin({ Config::BALL_RADIUS, Config::BALL_RADIUS });
	ball.setPosition(gameState.ballPosition);
	ball.setFillColor(sf::Color::White);
	window.draw(ball);

	// 1.5 Draw Cows (SANTI: COWS 29/04/26)
	// Cows are host-simulated obstacles. Client renders from packet only.
	for (const auto& cow : gameState.cows) {
		if (!cow.active) continue;

		if (mCowSprite) {
			const auto sz = mCowTexture.getSize();
			if (sz.x > 0 && sz.y > 0) {
				const float desired = Config::COW_RADIUS * 2.f;
				const float sx = desired / static_cast<float>(sz.x);
				const float sy = desired / static_cast<float>(sz.y);

				mCowSprite->setScale({ sx, sy });
				mCowSprite->setPosition(cow.position);
				window.draw(*mCowSprite);
				continue;
			}
		}

		// Fallback: simple circle if texture is missing.
		sf::CircleShape cowShape(Config::COW_RADIUS);
		cowShape.setOrigin({ Config::COW_RADIUS, Config::COW_RADIUS });
		cowShape.setPosition(cow.position);
		cowShape.setFillColor(sf::Color(110, 75, 35));
		window.draw(cowShape);
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
