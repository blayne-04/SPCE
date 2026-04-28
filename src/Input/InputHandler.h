#pragma once

#include "Network/NetworkManager.h"
#include "../Common/Packets.h"

// Left hand (movement)
constexpr sf::Keyboard::Key MOVE_UP_KEY = sf::Keyboard::Key::W;
constexpr sf::Keyboard::Key MOVE_DOWN_KEY = sf::Keyboard::Key::S;
constexpr sf::Keyboard::Key MOVE_LEFT_KEY = sf::Keyboard::Key::A;
constexpr sf::Keyboard::Key MOVE_RIGHT_KEY = sf::Keyboard::Key::D;

// Right hand (actions) – JKL cluster
constexpr sf::Keyboard::Key SHOOT_KEY = sf::Keyboard::Key::K;           // Shoot
constexpr sf::Keyboard::Key PASS_KEY = sf::Keyboard::Key::J;            // Pass
constexpr sf::Keyboard::Key TACKLE_KEY = sf::Keyboard::Key::L;          // Steal
constexpr sf::Keyboard::Key SWITCH_PLAYER_KEY = sf::Keyboard::Key::I;   // Swap Player

class InputHandler {

public:


	InputHandler() = default;
	/* Take inputs from a human player */
	InputPacket pollInput(int playerID);

};

InputPacket InputHandler::pollInput(int playerID) {
	InputPacket packet;
	packet.playerId = playerID;
	
	sf::Vector2f moveDirection(0.f, 0.f);
	if (sf::Keyboard::isKeyPressed(MOVE_UP_KEY)) {
		moveDirection.y -= 1.f;
	}
	if (sf::Keyboard::isKeyPressed(MOVE_DOWN_KEY)) {
		moveDirection.y += 1.f;
	}
	if (sf::Keyboard::isKeyPressed(MOVE_LEFT_KEY)) {
		moveDirection.x -= 1.f;
	}
	if (sf::Keyboard::isKeyPressed(MOVE_RIGHT_KEY)) {
		moveDirection.x += 1.f;
	}
	packet.moveDirection = moveDirection;

	packet.shootPressed = sf::Keyboard::isKeyPressed(SHOOT_KEY);
	packet.passPressed = sf::Keyboard::isKeyPressed(PASS_KEY);
	packet.stealPressed = sf::Keyboard::isKeyPressed(TACKLE_KEY);
	packet.lungePressed = sf::Keyboard::isKeyPressed(SWITCH_PLAYER_KEY);

	return packet;
}