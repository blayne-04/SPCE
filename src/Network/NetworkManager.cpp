#include "NetworkManager.h"

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

	// SANTI: unknown until first valid client JOIN_REQUEST or INPUT arrives.
	mRemoteAddress = sf::IpAddress::Any;
	mRemotePort = 0;
}

// ----------------------------------------------------------------------------
// CLIENT MODE: store host endpoint, bind to ephemeral local port
// ----------------------------------------------------------------------------
void NetworkManager::startClient(const sf::IpAddress& remoteAddress, uint16 remotePort)
{
	// SANTI: Store the host endpoint (destination for packets).
	mRemoteAddress = remoteAddress;
	mRemotePort = remotePort;

	mSocket.unbind();

	// SANTI: Client binds to a LOCAL ephemeral port to receive packets.
	const auto status = mSocket.bind(sf::Socket::AnyPort);
	if (status != sf::Socket::Status::Done) {
		mLocalPort = 0;
		return;
	}

	mLocalPort = static_cast<uint16>(mSocket.getLocalPort());
	mSocket.setBlocking(false);
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: send JOIN_REQUEST to host during handshake phase
// ----------------------------------------------------------------------------
void NetworkManager::sendJoinRequest()
{
	if (mRemotePort == 0) return;

	sf::Packet packet;
	packet << static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST);

	// SANTI: Send JOIN_REQUEST repeatedly until ASSIGNMENT received.
	// UDP is unreliable, so client should call this each frame until
	// pollIdAssignment() returns true.
	(void)mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: poll for ASSIGNMENT response from host
// ----------------------------------------------------------------------------
bool NetworkManager::pollIdAssignment(std::uint8_t& outId)
{
	sf::Packet receivedPacket;
	std::optional<sf::IpAddress> sender;
	unsigned short senderPort = 0;

	auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

	// SANTI: Drain socket looking for ASSIGNMENT message.
	// Only accept ASSIGNMENT from the expected host endpoint.
	while (receiveStatus != sf::Socket::Status::NotReady) {

		if (receiveStatus == sf::Socket::Status::Error) {
			return false;
		}

		if (receiveStatus != sf::Socket::Status::Done) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Validate sender is the host
		if (!sender.has_value() || *sender != mRemoteAddress || 
		    static_cast<uint16>(senderPort) != mRemotePort) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Read message type
		std::uint8_t messageType = 0;
		if (!(receivedPacket >> messageType)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Check if it's an ASSIGNMENT message
		if (messageType == static_cast<std::uint8_t>(NetMsg::ASSIGNMENT)) {
			std::uint8_t assignedId = 0;
			if (receivedPacket >> assignedId) {
				outId = assignedId;
				return true; // Success! Client is now assigned.
			}
		}

		// Continue draining (might be other packets like STATE)
		receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
	}

	return false; // No ASSIGNMENT received this frame
}

// ----------------------------------------------------------------------------
// HOST SIDE: check for JOIN_REQUEST and send ASSIGNMENT response
// ----------------------------------------------------------------------------
void NetworkManager::handleHandshakeRequests()
{
	sf::Packet receivedPacket;
	std::optional<sf::IpAddress> sender;
	unsigned short senderPort = 0;

	auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

	// SANTI: Drain socket looking for JOIN_REQUEST messages.
	// For single-client MVP, we always assign player ID 1 to first client.
	// Multi-client would need a connection table and free slot tracking.
	while (receiveStatus != sf::Socket::Status::NotReady) {

		if (receiveStatus == sf::Socket::Status::Error) {
			return;
		}

		if (receiveStatus != sf::Socket::Status::Done) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Read message type
		std::uint8_t messageType = 0;
		if (!(receivedPacket >> messageType)) {
			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
			continue;
		}

		// Check if it's a JOIN_REQUEST
		if (messageType == static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST)) {
			if (!sender.has_value()) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// SANTI: Single-client MVP - always assign player ID 1 (first away player).
			// This puts the client on the away team.
			// Host controls player 0 (home team).
			const std::uint8_t assignedPlayerId = 1;

			// Learn/update the client's endpoint
			mRemoteAddress = *sender;
			mRemotePort = static_cast<uint16>(senderPort);

			// Send ASSIGNMENT response
			sf::Packet responsePacket;
			responsePacket << static_cast<std::uint8_t>(NetMsg::ASSIGNMENT) << assignedPlayerId;
			
			(void)mSocket.send(responsePacket, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
		}

		// Continue draining socket
		receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
	}
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

		// SANTI: Handle JOIN_REQUEST during gameplay (late-join or reconnect)
		if (messageType == static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST)) {
			if (sender.has_value()) {
				const std::uint8_t assignedPlayerId = 1;
				mRemoteAddress = *sender;
				mRemotePort = static_cast<uint16>(senderPort);

				sf::Packet responsePacket;
				responsePacket << static_cast<std::uint8_t>(NetMsg::ASSIGNMENT) << assignedPlayerId;
				(void)mSocket.send(responsePacket, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
			}
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
