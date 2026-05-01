/**
 * @file InputHandler.cpp
 * @brief Keyboard-to-InputPacket conversion.
 */

#include "InputHandler.h"
#include "../Common/Constants.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

/**
 * @brief Read local keyboard state and return reliable DOWN-state input.
 *
 * The client sends button-down booleans every tick instead of one-frame press
 * edges. The authoritative host can later compare current and previous input
 * if it needs "pressed this frame" behavior.
 */
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
	// Accept both keys as tackle keys so quick testing doesn't get blocked by
	// different keybinding expectations.
	out.tackleDown =
		sf::Keyboard::isKeyPressed(Config::TACKLE_KEY) ||
		sf::Keyboard::isKeyPressed(Config::TACKLE_KEY_ALT);
	out.switchDown = sf::Keyboard::isKeyPressed(Config::SWITCH_PLAYER_KEY);

	/* Lunge key false for now */
	out.lungeDown = false;

	return out;
}
