#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>          // SANTI: added for sf::Packet
#include <array>
#include <cstdint>

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

// SANTI: changed from 10 to 8 players (4 per team)
inline constexpr std::size_t kNumPlayers = 8;

// SANTI: added NetMsg enum for packet type identification
enum class NetMsg : std::uint8_t {
	INPUT = 1,   // Client -> Host payload: InputPacket
	STATE = 2    // Host   -> Client payload: GameStatePacket
};

// ============================================================================
// UPLINK: Client -> Host
// ============================================================================

struct InputPacket {
	// SANTI: added inputSequence for reconciliation
	std::uint32_t inputSequence = 0;

	std::uint8_t playerId = 0;   // SANTI: changed type to uint8_t (same, but explicit)

	sf::Vector2f moveDirection{ 0.f, 0.f };

	// SANTI: removed aimTarget (host decides targets)
	// SANTI: renamed shootPressed -> shootDown, passPressed -> passDown
	// SANTI: renamed stealPressed -> tackleDown
	// SANTI: added switchDown (defender switch)
	// SANTI: renamed lungePressed -> lungeDown
	bool shootDown = false;
	bool passDown = false;
	bool tackleDown = false;
	bool switchDown = false;
	bool lungeDown = false;
};

struct FrameInput {
	// SANTI: changed from 10 to kNumPlayers (8)
	std::array<InputPacket, kNumPlayers> inputs{};
};

// ============================================================================
// DOWNLINK: Host -> Client
// ============================================================================

struct PlayerState {
	// SANTI: no changes to fields, only added default initializers and verbose serialization
	sf::Vector2f position{ 0.f, 0.f };
	sf::Vector2f velocity{ 0.f, 0.f };
	sf::Vector2f facingDirection{ 1.f, 0.f };
	std::uint8_t teamId = 0;
	bool isGoalkeeper = false;
	bool isLunging = false;
};

struct GameStatePacket {
	std::uint32_t frameNumber = 0;

	// SANTI: changed from 10 to kNumPlayers (8)
	std::array<PlayerState, kNumPlayers> players{};

	sf::Vector2f ballPosition{ 0.f, 0.f };
	sf::Vector2f ballVelocity{ 0.f, 0.f };

	std::int8_t ballOwnerPlayerId = -1;

	// SANTI: added possessingTeamId (stable possession during passes/shots)
	std::int8_t possessingTeamId = -1;

	// SANTI: added controlledHomePlayerId / controlledAwayPlayerId
	std::uint8_t controlledHomePlayerId = 0;
	std::uint8_t controlledAwayPlayerId = 4;

	std::uint16_t scoreHome = 0;
	std::uint16_t scoreAway = 0;

	// SANTI: renamed matchTimer -> matchTimerSec and changed semantic (counts UP, not down)
	float matchTimerSec = 0.f;


	sf::FloatRect pitchBounds{};    // Hard box boundaries of playable field

	std::uint8_t currentState = 0;
};

// ============================================================================
// SERIALIZATION (sf::Packet)
// ============================================================================
// SANTI: entire serialization section is new - original had no operators.
// ============================================================================

// SANTI: forward declare FloatRect serialization so GameStatePacket operators can use it
inline sf::Packet& operator<<(sf::Packet& packet, const sf::FloatRect& rect);
inline sf::Packet& operator>>(sf::Packet& packet, sf::FloatRect& rect);


inline sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector) {
	return packet << vector.x << vector.y;
}
inline sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector) {
	return packet >> vector.x >> vector.y;
}

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

inline sf::Packet& operator<<(sf::Packet& packet, const PlayerState& playerState) {
	return packet << playerState.position
		<< playerState.velocity
		<< playerState.facingDirection
		<< playerState.teamId
		<< playerState.isGoalkeeper
		<< playerState.isLunging;
}
inline sf::Packet& operator>>(sf::Packet& packet, PlayerState& playerState) {
	return packet >> playerState.position
		>> playerState.velocity
		>> playerState.facingDirection
		>> playerState.teamId
		>> playerState.isGoalkeeper
		>> playerState.isLunging;
}

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

// SANTI: added FloatRect serialization (SFML Packet has no built-in for this)
inline sf::Packet& operator<<(sf::Packet& packet, const sf::FloatRect& rect) {
	return packet << rect.position << rect.size;
}
inline sf::Packet& operator>>(sf::Packet& packet, sf::FloatRect& rect) {
	return packet >> rect.position >> rect.size;
}
