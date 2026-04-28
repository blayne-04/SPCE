#pragma once

// Forward declarations instead of includes
class Goal;
class Ball;
class Player;

class World
{
public:
    World() = default;
    ~World() = default;

    void overwriteWorldFromPacket(GameStatePacket incomingHostPacket);
    // Your methods here...

private:
    // Your member variables here...
};