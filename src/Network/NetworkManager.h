#pragma once
#include <SFML/Network.hpp>
#include "../Common/Packets.h"
#include <queue>
#include <vector>

using uint16 = unsigned short;

class NetworkManager {
public:
    NetworkManager();

    // --- INITIALIZATION ---
    void startHost(uint16 localPort);
    void startClient(const sf::IpAddress& hostAddress, uint16 hostPort);

    // --- HANDSHAKE PHASE (The "Chicken and Egg" Fix) ---

    /* CLIENT: Sends a "JoinRequest" packet to the Host. Call this once inside ClientPlayingState's constructor or first tick. */
    void sendJoinRequest();

    /* HOST: Polls for JoinRequests. If a new IP/Port connects, assigns them an ID (e.g., 5) and sends back an AssignmentPacket.*/
    void handleHandshakeRequests();

    /* CLIENT: Checks the socket for an AssignmentPacket. Returns true and fills outId if the Host has welcomed us. */
    bool pollIdAssignment(uint8_t& outId);


    // --- SIMULATION PHASE ---

    /* HOST: Collects all InputPackets from all connected clients.*/
    void pollIncomingInputs(std::queue<InputPacket>& inputQueue);

    /* HOST: Iterates through mClients and sends the World State to everyone. */
    void broadcastGameState(const GameStatePacket& gameStatePacket);

    /* CLIENT: Sends your local InputPacket to the mHostAddress. */
    void sendPlayerInput(const InputPacket& inputPacket);

    /* CLIENT: Checks for the latest authoritative "Truth" from the Host. */
    bool receiveLatestGameState(GameStatePacket& outState);

private:
    sf::UdpSocket mSocket;
    bool mIsHost = false;

    /* Client Data */
    sf::IpAddress mHostAddress;
    uint16 mHostPort;

    /* Host Data */
    struct ConnectedClient {
        sf::IpAddress ip;
        uint16 port;
        uint8_t playerId;
        sf::Time lastSeen;
    };

    std::vector<ConnectedClient> mClients;

    /* Check if a client is in our list */
    bool isClientKnown(const sf::IpAddress& ip, uint16 port);
};