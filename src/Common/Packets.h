#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>

// --- UPLINK: Client to Host ---
struct InputPacket {
    uint8_t playerId;           // Identifies the player (0-9)
    sf::Vector2f moveDirection; // Normalized WASD/Thumbstick vector
    sf::Vector2f aimTarget;     // World coordinates (Mouse/AI target point)
    bool shootPressed;          // Left Click (Field players only)
    bool passPressed;           // Right Click
    bool stealPressed;          // F Key (Steal/Block action)
    bool lungePressed;          // Space Key (Goalkeeper lunge - keeper only)
};

struct FrameInput {
    std::array<InputPacket, 10> inputs;
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
    std::array<PlayerState, 10> players;
    sf::Vector2f ballPosition;
    sf::Vector2f ballVelocity;
    int8_t ballOwnerPlayerId;   // -1 if loose
    uint16_t scoreHome;
    uint16_t scoreAway;
    float matchTimer;           // Counts down from match start time
    sf::FloatRect pitchBounds;  // Hard box boundaries of playable field
    uint8_t currentState;       // Enum: Kickoff, Playing, Goal, GameEnd
};