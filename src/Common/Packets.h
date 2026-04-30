#pragma once

/**
 * @file Packets.h
 * @brief Wire contract for the host-authoritative soccer game.
 *
 * AI disclosure:
 * The packet structure and field-by-field SFML serialization operators were
 * designed and documented with help from OpenAI Codex because this networking
 * contract is more advanced than typical CPTS 122 starter code.
 *
 * Prompt used:
 * "Help me design a host-authoritative UDP packet contract for an SFML soccer
 * game. Use GameState snapshots, InputPacket button-down state, no raw struct
 * memcpy, and sf::Packet serialization operators."
 */

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

/**
 * @brief Type tag at the front of every UDP packet.
 *
 * This prevents the receiver from accidentally parsing an input packet as a
 * game-state packet or vice versa.
 */
enum class NetMsg : std::uint8_t {
	INPUT = 1,        // Client -> Host payload: InputPacket
	STATE = 2,        // Host   -> Client payload: GameStatePacket
	JOIN_REQUEST = 3, // Client -> Host: request player ID assignment (no payload)
	ASSIGNMENT = 4    // Host   -> Client payload: assigned player ID (uint8_t)
};

// ============================================================================
// UPLINK: Client -> Host
// ============================================================================

/**
 * @brief Client-to-host input for one controlled player on one tick.
 *
 * The booleans are button DOWN state, not "pressed this frame". The host
 * computes press edges by comparing this tick against the previous tick.
 */
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

/**
 * @brief Host-side collection of every player's input for one simulation tick.
 */
struct FrameInput {
	std::array<InputPacket, Config::kNumPlayers> inputs{};
};

// ============================================================================
// DOWNLINK: Host -> Client
// ============================================================================

/**
 * @brief Snapshot of one player for rendering and client-side display.
 */
struct PlayerState {
	sf::Vector2f position{ 0.f, 0.f };
	sf::Vector2f velocity{ 0.f, 0.f };
	sf::Vector2f facingDirection{ 1.f, 0.f };
	std::uint8_t teamId = 0;
	bool isGoalkeeper = false;
	bool isLunging = false;
};

// SANTI: COWS 29/04/26
// Cow snapshot state. Cows are authoritative obstacles simulated on the host.
// Clients only render these states; they never simulate cow movement.
/**
 * @brief Snapshot of one cow obstacle.
 *
 * Cows are simulated by the host. Clients only render this packet state.
 */
struct CowState {
	bool active = false;
	sf::Vector2f position{ 0.f, 0.f };
	sf::Vector2f velocity{ 0.f, 0.f };
};

/**
 * @brief Host-to-client authoritative snapshot of the whole match.
 *
 * Renderer, networking, and client prediction avoidance all depend on this one
 * packet being stable and serialized field by field.
 */
struct GameStatePacket {
	std::uint32_t frameNumber = 0;

	std::array<PlayerState, Config::kNumPlayers> players{};

	// SANTI: COWS 29/04/26
	// Fixed-size cow array keeps networking deterministic and bandwidth-bounded.
	std::array<CowState, Config::kMaxCows> cows{};

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

// SANTI: COWS 29/04/26
inline sf::Packet& operator<<(sf::Packet& packet, const CowState& cowState) {
	return packet << cowState.active
		<< cowState.position
		<< cowState.velocity;
}
inline sf::Packet& operator>>(sf::Packet& packet, CowState& cowState) {
	return packet >> cowState.active
		>> cowState.position
		>> cowState.velocity;
}

/* Game state output */
inline sf::Packet& operator<<(sf::Packet& packet, const GameStatePacket& gameStatePacket) {
	packet << gameStatePacket.frameNumber;
	for (const auto& playerState : gameStatePacket.players) packet << playerState;
	for (const auto& cowState : gameStatePacket.cows) packet << cowState; // SANTI: COWS 29/04/26
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
	for (auto& cowState : gameStatePacket.cows) packet >> cowState; // SANTI: COWS 29/04/26
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
