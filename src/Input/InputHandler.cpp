#include "InputHandler.h"
#include "../Common/Constants.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

// Converts keyboard state into a network InputPacket (DOWN state, not edge-triggered).
InputPacket InputHandler::getLocalInput(std::uint8_t playerID)
{
	InputPacket out{};
	out.playerId = playerID;

	float x = 0.f;
	float y = 0.f;

	if (sf::Keyboard::isKeyPressed(Config::MOVE_LEFT_KEY))  x -= 1.f;
	if (sf::Keyboard::isKeyPressed(Config::MOVE_RIGHT_KEY)) x += 1.f;
	if (sf::Keyboard::isKeyPressed(Config::MOVE_UP_KEY))    y -= 1.f;
	if (sf::Keyboard::isKeyPressed(Config::MOVE_DOWN_KEY))  y += 1.f;


	// Normalize so diagonals aren't faster.
	const float lenSq = x * x + y * y;
	if (lenSq > 1.f) {
		const float invLen = 1.f / std::sqrt(lenSq);
		x *= invLen;
		y *= invLen;
	}

	out.moveDirection = sf::Vector2f(x, y);

	// Button DOWN semantics (host will compute "pressed this frame" if needed).
	out.shootDown = sf::Keyboard::isKeyPressed(Config::SHOOT_KEY);
	out.passDown = sf::Keyboard::isKeyPressed(Config::PASS_KEY);
	out.tackleDown = sf::Keyboard::isKeyPressed(Config::TACKLE_KEY);
	out.switchDown = sf::Keyboard::isKeyPressed(Config::SWITCH_PLAYER_KEY);

	// If you don't have a lunge key yet, keep it false for MVP.
	out.lungeDown = false;

	return out;
}