#include "Renderer.h"

#include "Common/Constants.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
const sf::Font* getHudFont() {
	static bool attempted = false;
	static sf::Font font;
	static bool loaded = false;

	if (attempted) {
		return loaded ? &font : nullptr;
	}

	attempted = true;

	// Config::FONT_PATH may be relative to the working directory.
	loaded = font.openFromFile(Config::FONT_PATH);
	if (loaded) {
		return &font;
	}

	// Fallback for build setups that copy assets into the executable directory.
	loaded = font.openFromFile("assets/fonts/arial.ttf");
	if (loaded) {
		return &font;
	}

	return nullptr;
}

sf::Color teamFillColor(std::uint8_t teamId) {
	if (teamId == 0) return sf::Color(235, 245, 255);
	return sf::Color(255, 230, 230);
}

sf::Color teamOutlineColor(std::uint8_t teamId) {
	if (teamId == 0) return sf::Color(60, 160, 255);
	return sf::Color(255, 90, 90);
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
	if (seconds < 0.f) seconds = 0.f;

	const int total = static_cast<int>(seconds);
	const int minutes = total / 60;
	const int secs = total % 60;

	std::ostringstream oss;
	oss << minutes << ":" << std::setfill('0') << std::setw(2) << secs;
	return oss.str();
}
} // namespace

void Renderer::render(sf::RenderWindow& window, const GameStatePacket& gameState) {
	// Map world coordinates (pitchBounds) into the window. With a full-viewport view,
	// resizing the window stretches the world to fill.
	sf::View worldView(gameState.pitchBounds);
	worldView.setViewport(sf::FloatRect({ 0.f, 0.f }, { 1.f, 1.f }));
	window.setView(worldView);

	// Pitch background.
	sf::RectangleShape pitch(sf::Vector2f(gameState.pitchBounds.size.x, gameState.pitchBounds.size.y));
	pitch.setPosition(gameState.pitchBounds.position);
	pitch.setFillColor(sf::Color(30, 110, 55));
	pitch.setOutlineThickness(2.f);
	pitch.setOutlineColor(sf::Color(15, 70, 35));
	window.draw(pitch);

	// Goals (debug visuals).
	sf::RectangleShape leftGoal(sf::Vector2f(Config::GOAL_WIDTH, Config::GOAL_HEIGHT));
	leftGoal.setPosition(sf::Vector2f(Config::LEFT_GOAL_X, Config::GOAL_Y_TOP));
	leftGoal.setFillColor(sf::Color(255, 255, 255, 40));
	leftGoal.setOutlineThickness(2.f);
	leftGoal.setOutlineColor(sf::Color(240, 240, 240, 180));
	window.draw(leftGoal);

	sf::RectangleShape rightGoal(sf::Vector2f(Config::GOAL_WIDTH, Config::GOAL_HEIGHT));
	rightGoal.setPosition(sf::Vector2f(Config::RIGHT_GOAL_X, Config::GOAL_Y_TOP));
	rightGoal.setFillColor(sf::Color(255, 255, 255, 40));
	rightGoal.setOutlineThickness(2.f);
	rightGoal.setOutlineColor(sf::Color(240, 240, 240, 180));
	window.draw(rightGoal);

	// Ball.
	sf::CircleShape ball(Config::BALL_RADIUS);
	ball.setOrigin(sf::Vector2f(Config::BALL_RADIUS, Config::BALL_RADIUS));
	ball.setPosition(gameState.ballPosition);
	ball.setFillColor(sf::Color(245, 245, 245));
	ball.setOutlineThickness(2.f);
	ball.setOutlineColor(sf::Color(10, 10, 10, 180));
	window.draw(ball);

	// Players.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const PlayerState& p = gameState.players[i];

		sf::CircleShape body(Config::PLAYER_HALF_SIZE);
		body.setOrigin(sf::Vector2f(Config::PLAYER_HALF_SIZE, Config::PLAYER_HALF_SIZE));
		body.setPosition(p.position);

		body.setFillColor(teamFillColor(p.teamId));
		body.setOutlineThickness(2.f);
		body.setOutlineColor(teamOutlineColor(p.teamId));

		// Highlight controlled players.
		const bool isControlledHome = (static_cast<std::uint8_t>(i) == gameState.controlledHomePlayerId);
		const bool isControlledAway = (static_cast<std::uint8_t>(i) == gameState.controlledAwayPlayerId);
		if (isControlledHome || isControlledAway) {
			body.setOutlineThickness(4.f);
			body.setOutlineColor(sf::Color(255, 215, 0));
		}

		// Goalkeeper marker (visual only).
		if (p.isGoalkeeper) {
			body.setFillColor(sf::Color(240, 240, 170));
		}

		window.draw(body);

		// Facing direction line.
		const sf::Vector2f start = p.position;
		const sf::Vector2f end = p.position + (p.facingDirection * (Config::PLAYER_HALF_SIZE * 1.4f));
		sf::VertexArray facing(sf::PrimitiveType::Lines, 2);
		facing[0].position = start;
		facing[1].position = end;
		facing[0].color = sf::Color(20, 20, 20, 200);
		facing[1].color = sf::Color(20, 20, 20, 200);
		window.draw(facing);
	}

	// HUD in screen space (not world space).
	window.setView(window.getDefaultView());
	renderHUD(window,
		static_cast<int>(gameState.scoreHome),
		static_cast<int>(gameState.scoreAway),
		gameState.matchTimerSec,
		static_cast<int>(gameState.currentState));
}

void Renderer::renderHUD(sf::RenderWindow& window, int homeScore, int awayScore, float matchTimerSec, int stateID) {
	const sf::Font* font = getHudFont();
	if (!font) return;

	const std::string scoreStr = "HOME " + std::to_string(homeScore) + " - " + std::to_string(awayScore) + " AWAY";
	sf::Text scoreText(*font, scoreStr, Config::SCORE_TEXT_SIZE);
	scoreText.setFillColor(sf::Color::White);
	scoreText.setPosition(sf::Vector2f(Config::SCORE_TEXT_X, Config::SCORE_TEXT_Y));
	window.draw(scoreText);

	const std::string timeStr = "TIME " + formatClock(matchTimerSec);
	sf::Text timeText(*font, timeStr, Config::SCORE_TEXT_SIZE);
	timeText.setFillColor(sf::Color::White);
	timeText.setPosition(sf::Vector2f(Config::SCORE_TEXT_X, Config::SCORE_TEXT_Y + 28.f));
	window.draw(timeText);

	const std::string stateStr = stateNameFromId(stateID);
	sf::Text stateText(*font, stateStr, Config::STATE_TEXT_SIZE);
	stateText.setFillColor(sf::Color::White);
	stateText.setPosition(sf::Vector2f(Config::STATE_TEXT_X, Config::STATE_TEXT_Y));
	window.draw(stateText);
}
