#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>

// NEW (santi)
inline constexpr std::size_t kNumPlayers = 8; // Total number of players in the match (4 per team)
//

// --- UPLINK: Client to Host ---
struct InputPacket {

	// NEW (santi)
	std::uint32_t inputSequence; // Incrementing sequence number for input packets, used for client-side reconciliation
	//

	uint8_t playerId;           // Identifies the player (0-7) NOTE: Player 0-3 = Home, Player 4-7 = Away There are only 8 players. 3 outfield, 1 goalkeeper per team. (Santi)
	sf::Vector2f moveDirection; // Normalized WASD/Thumbstick vector
	sf::Vector2f aimTarget;     // World coordinates (Mouse/AI target point)
	bool shootPressed;          // Left Click (Field players only)
	bool passPressed;           // Right Click
	bool stealPressed;          // F Key (Steal/Block action)
	bool lungePressed;          // Space Key (Goalkeeper lunge - keeper only)
};

struct FrameInput {
	std::array<InputPacket, kNumPlayers> inputs;
};

// --- DOWNLINK: Host to Client ---
struct PlayerState {
	sf::Vector2f position;
	sf::Vector2f velocity;
	sf::Vector2f facingDirection; // Derived from aimTarget - position
	uint8_t teamId;             // 0 = Home, 1 = Away
	bool isGoalkeeper;
	bool isLunging;             // Visual feedback for goalkeeper lunge state
};

struct GameStatePacket {
	uint32_t frameNumber;
	std::array<PlayerState, kNumPlayers> players;
	sf::Vector2f ballPosition;
	sf::Vector2f ballVelocity;
	int8_t ballOwnerPlayerId;   // -1 if loose

	// new (santi) I'll explain in a second
	int8_t possessingTeamId;
	uint8_t controlledHomePlayerId;
	uint8_t controlledAwayPlayerId;
	uint16_t scoreHome;
	uint16_t scoreAway;
	float matchTimerSec;           // Counts UP from match start time
	std::uint8_t currentState;       // Enum: Kickoff, Playing, Goal, GameEnd
	//

	uint16_t scoreHome;
	uint16_t scoreAway;
	float matchTimer;           // Counts down from match start time
	sf::FloatRect pitchBounds;  // Hard box boundaries of playable field
	uint8_t currentState;       // Enum: Kickoff, Playing, Goal, GameEnd
};



