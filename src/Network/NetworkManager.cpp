#include "NetworkManager.h"
#include <optional>

// ============================================================================
// NETWORK MANAGER
// ============================================================================
// Role: The "mailroom" of the architecture.
//
// Responsibilities:
//   - Owns the UDP socket and packet serialization (sf::UdpSocket, sf::Packet).
//   - Implements the wire protocol defined in Common/Packets.h:
//        NetMsg (first byte tag)
//        InputPacket (client -> host)
//        GameStatePacket (host -> client)
//   - Converts between raw UDP datagrams and structured packets.
//
// Non-responsibilities (must NOT know about):
//   - Game simulation (Match, World, etc.)
//   - Rendering (Renderer)
//   - AI logic
//   - Input generation (InputHandler)
//
// Architecture invariants:
//   - Non-blocking socket (polling inside game loop).
//   - Tagged datagrams: first byte = NetMsg::INPUT or NetMsg::STATE.
//   - Single-client MVP (host only talks to one client).
//   - Host learns client endpoint from first received INPUT packet.
// ============================================================================



// ----------------------------------------------------------------------------
// HOST MODE: bind to a fixed port, wait for client inputs
// ----------------------------------------------------------------------------
void NetworkManager::startHost(uint16 localPort)
{
	mSocket.unbind();

	const auto status = mSocket.bind(static_cast<unsigned short>(localPort));
	if (status != sf::Socket::Status::Done) {
		// SANTI: bind failed -> socket unusable; keep defaults.
		mLocalPort = 0;
		mRemoteAddress = sf::IpAddress::Any;
		mRemotePort = 0;
		return;
	}

	mLocalPort = localPort;
	mSocket.setBlocking(false);

	// SANTI: unknown until first valid client INPUT arrives.
	mRemoteAddress = sf::IpAddress::Any;
	mRemotePort = 0;
}

// ----------------------------------------------------------------------------
// CLIENT MODE: store host endpoint, bind to ephemeral local port
// ----------------------------------------------------------------------------
void NetworkManager::startClient(const sf::IpAddress& remoteAddress, uint16 remotePort)
{
	// SANTI: Store the host endpoint (destination for INPUT packets).
	mRemoteAddress = remoteAddress;
	mRemotePort = remotePort;

	mSocket.unbind();

	// SANTI: Client binds to a LOCAL ephemeral port to receive STATE snapshots.
	const auto status = mSocket.bind(sf::Socket::AnyPort);
	if (status != sf::Socket::Status::Done) {
		mLocalPort = 0;
		return;
	}

	mLocalPort = static_cast<uint16>(mSocket.getLocalPort());
	mSocket.setBlocking(false);
}

// ----------------------------------------------------------------------------
// HOST SIDE: drain UDP receive buffer, extract all valid InputPackets
// ----------------------------------------------------------------------------
void NetworkManager::pollIncomingInputs(std::queue<InputPacket>& outQueue)
{
	sf::Packet receivedPacket;
	std::optional<sf::IpAddress> sender;
	unsigned short senderPort = 0;

	auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

	while (receiveStatus != sf::Socket::Status::NotReady) {

		// SANTI: Avoid infinite spin if the socket errors.
		if (receiveStatus == sf::Socket::Status::Error) {
			return;
		}

		// Guard: not Done -> skip, fetch next, continue
		if (receiveStatus != sf::Socket::Status::Done) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Guard: failed to read message type
		std::uint8_t messageType = 0;
		if (!(receivedPacket >> messageType)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Guard: wrong message type (we only accept INPUT from clients)
		if (messageType != static_cast<std::uint8_t>(NetMsg::INPUT)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Guard: failed to deserialize InputPacket
		InputPacket inputPacket{};
		if (!(receivedPacket >> inputPacket)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Guard: invalid player ID
		// SANTI: kNumPlayers is defined in Config (Common/Constants.h).
		if (inputPacket.playerId >= Config::kNumPlayers) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		outQueue.push(inputPacket);

		// SANTI: Single-client MVP behavior.
		// Host learns/refreshes the remote endpoint from received INPUT packets.
		if (sender.has_value()) {
			mRemoteAddress = *sender;
			mRemotePort = static_cast<uint16>(senderPort);
		}

		receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
	}
}

// ----------------------------------------------------------------------------
// HOST SIDE: send authoritative snapshot to the client
// ----------------------------------------------------------------------------
void NetworkManager::sendGameState(const GameStatePacket& gameStatePacket)
{
	if (mRemotePort == 0) return;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(NetMsg::STATE) << gameStatePacket;

	// SANTI: UDP send can fail; MVP ignores return status (log later if needed).
	(void)mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: send local input to the host
// ----------------------------------------------------------------------------
void NetworkManager::sendPlayerInput(const InputPacket& inputPacket)
{
	if (mRemotePort == 0) return;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(NetMsg::INPUT) << inputPacket;

	(void)mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: receive snapshots, keep only the newest (by frameNumber)
// ----------------------------------------------------------------------------
bool NetworkManager::receiveLatestGameState(GameStatePacket& outState)
{
	sf::Packet receivedPacket;
	std::optional<sf::IpAddress> sender;
	unsigned short senderPort = 0;

	bool gotAny = false;

	auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

	while (receiveStatus != sf::Socket::Status::NotReady) {

		if (receiveStatus == sf::Socket::Status::Error) {
			return gotAny;
		}

		if (receiveStatus != sf::Socket::Status::Done) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		std::uint8_t messageType = 0;
		if (!(receivedPacket >> messageType)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		if (messageType != static_cast<std::uint8_t>(NetMsg::STATE)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// SANTI: Optional noise filter (ignore non-host senders).
		if (sender.has_value()) {
			if (*sender != mRemoteAddress) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}
		}
		if (static_cast<uint16>(senderPort) != mRemotePort) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		GameStatePacket state{};
		if (!(receivedPacket >> state)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// SANTI: Keep newest snapshot even if UDP reorders packets.
		// Note: wrap-aware comparison could be added later if frameNumber ever wraps.
		if (!gotAny || state.frameNumber > outState.frameNumber) {
			outState = state;
		}
		gotAny = true;

		receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
	}

	return gotAny;
}
