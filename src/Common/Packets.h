#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>          // SANTI: added for sf::Packet
#include <array>
#include <cstdint>

#include "Constants.h"

// ============================================================================
// NETWORK PROTOCOL (HOST-AUTHORITATIVE, SNAPSHOT-DRIVEN)
// ============================================================================
//
// Core rules:
// 1) Player IDs: 0-3 = Home, 4-7 = Away (8 players total; 4 per team).
// 2) Host is authoritative: ...
// 3) This header is the wire contract.
// 4) No raw struct memcpy. Always serialize field-by-field with sf::Packet.
//
// Input reliability rule (important for UDP):
// - Send button DOWN state each tick (shootDown/passDown/etc).
// - Host computes "pressed this frame" by comparing with last tick's input.
// ============================================================================

enum class NetMsg : std::uint8_t {
	INPUT = 1,        // Client -> Host payload: InputPacket
	STATE = 2,        // Host   -> Client payload: GameStatePacket
	JOIN_REQUEST = 3, // Client -> Host: request player ID assignment (no payload)
	ASSIGNMENT = 4    // Host   -> Client payload: assigned player ID (uint8_t)
};

// ============================================================================
// UPLINK: Client -> Host
// ============================================================================

struct InputPacket {
	std::uint32_t inputSequence = 0;

	std::uint8_t playerId = 0;

	sf::Vector2f moveDirection{ 0.f, 0.f };

	bool shootDown = false;
	bool passDown = false;
	bool tackleDown = false;
	bool switchDown = false;
	bool lungeDown = false;
};

struct FrameInput {
	std::array<InputPacket, Config::kNumPlayers> inputs{};
};

// ============================================================================
// DOWNLINK: Host -> Client
// ============================================================================

struct PlayerState {
	sf::Vector2f position{ 0.f, 0.f };
	sf::Vector2f velocity{ 0.f, 0.f };
	sf::Vector2f facingDirection{ 1.f, 0.f };
	std::uint8_t teamId = 0;
	bool isGoalkeeper = false;
	bool isLunging = false;
};

struct GameStatePacket {
	std::uint32_t frameNumber = 0;

	std::array<PlayerState, Config::kNumPlayers> players{};

	sf::Vector2f ballPosition{ 0.f, 0.f };
	sf::Vector2f ballVelocity{ 0.f, 0.f };

	std::int8_t ballOwnerPlayerId = -1;

	std::int8_t possessingTeamId = -1;

	std::uint8_t controlledHomePlayerId = 0;
	std::uint8_t controlledAwayPlayerId = 4;

	std::uint16_t scoreHome = 0;
	std::uint16_t scoreAway = 0;

	float matchTimerSec = 0.f;


	sf::FloatRect pitchBounds{};

	std::uint8_t currentState = 0;
};

// ============================================================================
// SERIALIZATION (sf::Packet
// ============================================================================

inline sf::Packet& operator<<(sf::Packet& packet, const sf::FloatRect& rect);
inline sf::Packet& operator>>(sf::Packet& packet, sf::FloatRect& rect);


inline sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector) {
	return packet << vector.x << vector.y;
}
inline sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector) {
	return packet >> vector.x >> vector.y;
}

/* Input packet output*/
inline sf::Packet& operator<<(sf::Packet& packet, const InputPacket& inputPacket) {
	return packet << inputPacket.inputSequence
		<< inputPacket.playerId
		<< inputPacket.moveDirection
		<< inputPacket.shootDown
		<< inputPacket.passDown
		<< inputPacket.tackleDown
		<< inputPacket.switchDown
		<< inputPacket.lungeDown;
}

/* Input packet input */
inline sf::Packet& operator>>(sf::Packet& packet, InputPacket& inputPacket) {
	return packet >> inputPacket.inputSequence
		>> inputPacket.playerId
		>> inputPacket.moveDirection
		>> inputPacket.shootDown
		>> inputPacket.passDown
		>> inputPacket.tackleDown
		>> inputPacket.switchDown
		>> inputPacket.lungeDown;
}

/* Player state output */
inline sf::Packet& operator<<(sf::Packet& packet, const PlayerState& playerState) {
	return packet << playerState.position
		<< playerState.velocity
		<< playerState.facingDirection
		<< playerState.teamId
		<< playerState.isGoalkeeper
		<< playerState.isLunging;
}

/* Player state input */
inline sf::Packet& operator>>(sf::Packet& packet, PlayerState& playerState) {
	return packet >> playerState.position
		>> playerState.velocity
		>> playerState.facingDirection
		>> playerState.teamId
		>> playerState.isGoalkeeper
		>> playerState.isLunging;
}

/* Game state output */
inline sf::Packet& operator<<(sf::Packet& packet, const GameStatePacket& gameStatePacket) {
	packet << gameStatePacket.frameNumber;
	for (const auto& playerState : gameStatePacket.players) packet << playerState;
	packet << gameStatePacket.ballPosition
		<< gameStatePacket.ballVelocity
		<< gameStatePacket.ballOwnerPlayerId
		<< gameStatePacket.possessingTeamId
		<< gameStatePacket.controlledHomePlayerId
		<< gameStatePacket.controlledAwayPlayerId
		<< gameStatePacket.scoreHome
		<< gameStatePacket.scoreAway
		<< gameStatePacket.matchTimerSec
		<< gameStatePacket.pitchBounds
		<< gameStatePacket.currentState;
	return packet;
}
/* Game state input */
inline sf::Packet& operator>>(sf::Packet& packet, GameStatePacket& gameStatePacket) {
	packet >> gameStatePacket.frameNumber;
	for (auto& playerState : gameStatePacket.players) packet >> playerState;
	packet >> gameStatePacket.ballPosition
		>> gameStatePacket.ballVelocity
		>> gameStatePacket.ballOwnerPlayerId
		>> gameStatePacket.possessingTeamId
		>> gameStatePacket.controlledHomePlayerId
		>> gameStatePacket.controlledAwayPlayerId
		>> gameStatePacket.scoreHome
		>> gameStatePacket.scoreAway
		>> gameStatePacket.matchTimerSec
		>> gameStatePacket.pitchBounds
		>> gameStatePacket.currentState;
	return packet;
}

/* Float Rect Serialization */
inline sf::Packet& operator<<(sf::Packet& packet, const sf::FloatRect& rect) {
	return packet << rect.position << rect.size;
}
inline sf::Packet& operator>>(sf::Packet& packet, sf::FloatRect& rect) {
	return packet >> rect.position >> rect.size;
}
