#include "NetworkManager.h"

/**
 * @file NetworkManager.cpp
 * @brief UDP send/receive implementation for host and client modes.
 *
 * AI assistance disclosure:
 * A generative AI assistant (Codex) was used in a limited way to review non-blocking UDP receive patterns and
 * to sanity-check the packet type tagging approach (so INPUT/STATE packets are not mis-parsed). The
 * team implemented the networking behavior and validated it via local host/client play-testing.
 *
 * Prompt used:
 * "Review this UDP send/receive implementation. Suggest safe non-blocking polling patterns and how
 * to keep only the latest STATE snapshot, without adding gameplay rules."
 */

#include <iostream>
#include <queue>

 // ----------------------------------------------------------------------------
 // STOP NETWORK SESSION
 // ----------------------------------------------------------------------------
void NetworkManager::stop()
{
	// SFML UDP has no "disconnect" for this design because we use datagrams.
	// Ending the session means unbinding the socket and clearing the remembered
	// local/remote endpoints so future modes start from a clean network state.
	mSocket.unbind();
	mLocalPort = 0;
	mRemoteAddress = sf::IpAddress::Any;
	mRemotePort = 0;
}

// ----------------------------------------------------------------------------
// HOST MODE: bind to a fixed port
// ----------------------------------------------------------------------------
void NetworkManager::startHost(uint16 localPort)
{
	try {
		mSocket.unbind();

		std::cout << "[HOST] Attempting to bind to port " << localPort << "..." << std::endl;

		const auto status = mSocket.bind(static_cast<unsigned short>(localPort));

		if (status != sf::Socket::Status::Done) {
			std::cerr << "[HOST ERROR] Failed to bind socket to port " << localPort << std::endl;
			std::cerr << "[HOST ERROR] Status code: " << static_cast<int>(status) << std::endl;
			std::cerr << "[HOST ERROR] Possible causes:" << std::endl;
			std::cerr << "  - Port already in use by another application" << std::endl;
			std::cerr << "  - Insufficient permissions" << std::endl;
			std::cerr << "  - Firewall blocking the port" << std::endl;

			// Reset to unusable state
			mLocalPort = 0;
			mRemoteAddress = sf::IpAddress::Any;
			mRemotePort = 0;
			return;
		}

		mLocalPort = localPort;
		mSocket.setBlocking(false);

		// Unknown until first valid client JOIN_REQUEST or INPUT arrives
		mRemoteAddress = sf::IpAddress::Any;
		mRemotePort = 0;

		std::optional<sf::IpAddress> myIP = sf::IpAddress::getLocalAddress();
		std::cout << "[HOST] Successfully bound to port " << localPort << std::endl;
		if (myIP) {
			std::cout << "[HOST] Your IP address is: " << myIP->toString() << std::endl;
			std::cout << "[HOST] Clients should connect to: " << myIP->toString() << ":" << localPort << std::endl;
		}
		std::cout << "[HOST] Socket is non-blocking and ready to receive" << std::endl;
		std::cout << "[HOST] Waiting for JOIN_REQUEST packets..." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[HOST EXCEPTION] startHost failed: " << e.what() << std::endl;
		mLocalPort = 0;
	}
	catch (...) {
		std::cerr << "[HOST EXCEPTION] startHost failed with unknown error" << std::endl;
		mLocalPort = 0;
	}
}

// ----------------------------------------------------------------------------
// CLIENT MODE: store host endpoint, bind to ephemeral local port
// ----------------------------------------------------------------------------
void NetworkManager::startClient(const sf::IpAddress& remoteAddress, uint16 remotePort)
{
	try {
		std::cout << "[CLIENT] ========================================" << std::endl;
		std::cout << "[CLIENT] Attempting to connect to host" << std::endl;
		std::cout << "[CLIENT] Target: " << remoteAddress.toString() << ":" << remotePort << std::endl;

		// Store the host endpoint (destination for packets)
		mRemoteAddress = remoteAddress;
		mRemotePort = remotePort;

		mSocket.unbind();

		std::cout << "[CLIENT] Binding to local ephemeral port..." << std::endl;

		// Client binds to a LOCAL ephemeral port to receive packets
		const auto status = mSocket.bind(sf::Socket::AnyPort);

		if (status != sf::Socket::Status::Done) {
			std::cerr << "[CLIENT ERROR] Failed to bind to local port" << std::endl;
			std::cerr << "[CLIENT ERROR] Status code: " << static_cast<int>(status) << std::endl;
			std::cerr << "[CLIENT ERROR] Cannot receive packets from host!" << std::endl;
			mLocalPort = 0;
			return;
		}

		mLocalPort = static_cast<uint16>(mSocket.getLocalPort());
		mSocket.setBlocking(false);

		std::optional<sf::IpAddress> myIP = sf::IpAddress::getLocalAddress();
		std::cout << "[CLIENT] Successfully bound to local port " << mLocalPort << std::endl;
		if (myIP) {
			std::cout << "[CLIENT] My IP address: " << myIP->toString() << std::endl;
		}
		std::cout << "[CLIENT] Sending packets to: " << mRemoteAddress.toString()
			<< ":" << mRemotePort << std::endl;
		std::cout << "[CLIENT] Ready to send/receive packets" << std::endl;
		std::cout << "[CLIENT] ========================================" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "[CLIENT EXCEPTION] startClient failed: " << e.what() << std::endl;
		mLocalPort = 0;
	}
	catch (...) {
		std::cerr << "[CLIENT EXCEPTION] startClient failed with unknown error" << std::endl;
		mLocalPort = 0;
	}
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: send JOIN_REQUEST to host during handshake phase
// ----------------------------------------------------------------------------
void NetworkManager::sendJoinRequest()
{
	try {
		if (mRemotePort == 0) {
			std::cerr << "[CLIENT ERROR] sendJoinRequest: Remote port not set! Did startClient() fail?" << std::endl;
			return;
		}

		sf::Packet packet;
		packet << static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST);

		auto sendStatus = mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));

		static int requestCount = 0;
		static int failureCount = 0;

		if (sendStatus != sf::Socket::Status::Done) {
			failureCount++;
			if (failureCount % 60 == 0) {
				std::cerr << "[CLIENT ERROR] JOIN_REQUEST send failed " << failureCount << " times!" << std::endl;
				std::cerr << "[CLIENT ERROR] Status: " << static_cast<int>(sendStatus) << std::endl;
				std::cerr << "[CLIENT ERROR] This suggests:" << std::endl;
				std::cerr << "  1. Host IP address may be wrong: " << mRemoteAddress.toString() << std::endl;
				std::cerr << "  2. Host may not be reachable on this network" << std::endl;
				std::cerr << "  3. Firewall is blocking outbound UDP" << std::endl;
				std::cerr << "  4. School network may block P2P connections" << std::endl;
			}
		}
		else {
			requestCount++;
			if (requestCount == 1) {
				std::cout << "[CLIENT] First JOIN_REQUEST sent successfully" << std::endl;
			}
			if (requestCount % 120 == 0) { // Every 2 seconds
				std::cout << "[CLIENT] Still waiting for response... (sent " << requestCount << " requests)" << std::endl;
				std::cout << "[CLIENT] Host should show: 'Received JOIN_REQUEST from <your_ip>'" << std::endl;
			}
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[CLIENT EXCEPTION] sendJoinRequest: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[CLIENT EXCEPTION] sendJoinRequest: Unknown error" << std::endl;
	}
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: poll for ASSIGNMENT response from host
// ----------------------------------------------------------------------------
bool NetworkManager::pollIdAssignment(std::uint8_t& outId)
{
	try {
		sf::Packet receivedPacket;
		std::optional<sf::IpAddress> sender;
		unsigned short senderPort = 0;

		auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

		static int totalPacketsReceived = 0;

		// Drain socket looking for ASSIGNMENT message
		while (receiveStatus != sf::Socket::Status::NotReady) {

			if (receiveStatus == sf::Socket::Status::Error) {
				std::cerr << "[CLIENT ERROR] Socket receive error in pollIdAssignment" << std::endl;
				return false;
			}

			if (receiveStatus != sf::Socket::Status::Done) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			totalPacketsReceived++;

			// ANY packet received is good news - means network is working
			if (totalPacketsReceived == 1) {
				std::cout << "[CLIENT] First packet received! Network is working!" << std::endl;
			}

			// Validate sender
			if (!sender.has_value()) {
				std::cerr << "[CLIENT WARNING] Received packet with no sender address" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Log all received packets to debug routing
			std::cout << "[CLIENT] Received packet from " << sender->toString() << ":" << senderPort << std::endl;

			if (*sender != mRemoteAddress || static_cast<uint16>(senderPort) != mRemotePort) {
				std::cerr << "[CLIENT WARNING] Packet from unexpected source!" << std::endl;
				std::cerr << "  Received from: " << sender->toString() << ":" << senderPort << std::endl;
				std::cerr << "  Expected from: " << mRemoteAddress.toString() << ":" << mRemotePort << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Read message type
			std::uint8_t messageType = 0;
			if (!(receivedPacket >> messageType)) {
				std::cerr << "[CLIENT WARNING] Failed to read message type from packet" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			std::cout << "[CLIENT] Message type: " << static_cast<int>(messageType) << std::endl;

			// Check if it's an ASSIGNMENT message
			if (messageType == static_cast<std::uint8_t>(NetMsg::ASSIGNMENT)) {
				std::uint8_t assignedId = 0;
				if (receivedPacket >> assignedId) {
					outId = assignedId;
					std::cout << "[CLIENT SUCCESS] ========================================" << std::endl;
					std::cout << "[CLIENT SUCCESS] Connected! Player ID: " << static_cast<int>(assignedId) << std::endl;
					std::cout << "[CLIENT SUCCESS] ========================================" << std::endl;
					return true;
				}
				else {
					std::cerr << "[CLIENT ERROR] Received ASSIGNMENT but failed to parse ID" << std::endl;
				}
			}
			else {
				std::cout << "[CLIENT INFO] Received message type " << static_cast<int>(messageType)
					<< " (expected ASSIGNMENT=" << static_cast<int>(NetMsg::ASSIGNMENT) << ")" << std::endl;
			}

			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
		}

		return false; // No ASSIGNMENT received this frame
	}
	catch (const std::exception& e) {
		std::cerr << "[CLIENT EXCEPTION] pollIdAssignment: " << e.what() << std::endl;
		return false;
	}
	catch (...) {
		std::cerr << "[CLIENT EXCEPTION] pollIdAssignment: Unknown error" << std::endl;
		return false;
	}
}

// ----------------------------------------------------------------------------
// HOST SIDE: drain UDP receive buffer, extract all valid InputPackets
// ----------------------------------------------------------------------------
void NetworkManager::pollIncomingInputs(std::queue<InputPacket>& outQueue)
{
	try {
		sf::Packet receivedPacket;
		std::optional<sf::IpAddress> sender;
		unsigned short senderPort = 0;

		auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

		while (receiveStatus != sf::Socket::Status::NotReady) {

			if (receiveStatus == sf::Socket::Status::Error) {
				std::cerr << "[HOST ERROR] Socket receive error in pollIncomingInputs" << std::endl;
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

			// Handle JOIN_REQUEST during gameplay (late-join or reconnect)
			if (messageType == static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST)) {
				if (sender.has_value()) {
					const std::uint8_t assignedPlayerId = 4;
					mRemoteAddress = *sender;
					mRemotePort = static_cast<uint16>(senderPort);

					std::cout << "[HOST] Late JOIN_REQUEST from " << sender->toString()
						<< ":" << senderPort << std::endl;

					sf::Packet responsePacket;
					responsePacket << static_cast<std::uint8_t>(NetMsg::ASSIGNMENT) << assignedPlayerId;
					mSocket.send(responsePacket, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
				}
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Accept INPUT messages
			if (messageType != static_cast<std::uint8_t>(NetMsg::INPUT)) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Deserialize InputPacket
			InputPacket inputPacket{};
			if (!(receivedPacket >> inputPacket)) {
				std::cerr << "[HOST WARNING] Failed to deserialize INPUT packet" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Validate player ID
			if (inputPacket.playerId >= Config::kNumPlayers) {
				std::cerr << "[HOST WARNING] Received INPUT with invalid player ID: "
					<< static_cast<int>(inputPacket.playerId) << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			outQueue.push(inputPacket);

			// Learn/refresh the remote endpoint
			if (sender.has_value()) {
				mRemoteAddress = *sender;
				mRemotePort = static_cast<uint16>(senderPort);
			}

			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[HOST EXCEPTION] pollIncomingInputs: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[HOST EXCEPTION] pollIncomingInputs: Unknown error" << std::endl;
	}
}

// ----------------------------------------------------------------------------
// HOST SIDE: send authoritative snapshot to the client
// ----------------------------------------------------------------------------
void NetworkManager::sendGameState(const GameStatePacket& gameStatePacket)
{
	try {
		if (mRemotePort == 0) {
			return;
		}

		sf::Packet packet;
		packet << static_cast<std::uint8_t>(NetMsg::STATE) << gameStatePacket;

		auto sendStatus = mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));

		if (sendStatus != sf::Socket::Status::Done) {
			static int errorCount = 0;
			if (errorCount % 60 == 0) {
				std::cerr << "[HOST WARNING] Failed to send game state. Status: "
					<< static_cast<int>(sendStatus) << std::endl;
			}
			errorCount++;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[HOST EXCEPTION] sendGameState: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[HOST EXCEPTION] sendGameState: Unknown error" << std::endl;
	}
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: send local input to the host
// ----------------------------------------------------------------------------
void NetworkManager::sendPlayerInput(const InputPacket& inputPacket)
{
	try {
		if (mRemotePort == 0) {
			std::cerr << "[CLIENT ERROR] sendPlayerInput: Remote port not set!" << std::endl;
			return;
		}

		sf::Packet packet;
		packet << static_cast<std::uint8_t>(NetMsg::INPUT) << inputPacket;

		auto sendStatus = mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));

		if (sendStatus != sf::Socket::Status::Done) {
			static int errorCount = 0;
			if (errorCount % 60 == 0) {
				std::cerr << "[CLIENT WARNING] Failed to send input. Status: "
					<< static_cast<int>(sendStatus) << std::endl;
			}
			errorCount++;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[CLIENT EXCEPTION] sendPlayerInput: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[CLIENT EXCEPTION] sendPlayerInput: Unknown error" << std::endl;
	}
}

// ----------------------------------------------------------------------------
// HOST SIDE: report whether a remote endpoint has been learned
// ----------------------------------------------------------------------------
bool NetworkManager::hasRemoteClient() const
{
	// UDP does not have a built-in connected/disconnected flag. mRemotePort is
	// reset to 0 in startHost() and becomes nonzero only after the host receives
	// a valid JOIN_REQUEST or INPUT packet from the client.
	return mRemotePort != 0;
}

// ----------------------------------------------------------------------------
// CLIENT SIDE: receive snapshots, keep only the newest (by frameNumber)
// ----------------------------------------------------------------------------
bool NetworkManager::receiveLatestGameState(GameStatePacket& outState)
{
	try {
		sf::Packet receivedPacket;
		std::optional<sf::IpAddress> sender;
		unsigned short senderPort = 0;

		bool gotAny = false;
		int packetCount = 0;

		auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

		while (receiveStatus != sf::Socket::Status::NotReady) {
			packetCount++;

			if (receiveStatus == sf::Socket::Status::Error) {
				std::cerr << "[CLIENT ERROR] Socket receive error in receiveLatestGameState" << std::endl;
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

			if (sender.has_value() && *sender != mRemoteAddress) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			if (static_cast<uint16>(senderPort) != mRemotePort) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			GameStatePacket state{};
			if (!(receivedPacket >> state)) {
				std::cerr << "[CLIENT WARNING] Failed to deserialize GameStatePacket" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			if (!gotAny || state.frameNumber > outState.frameNumber) {
				outState = state;
			}
			gotAny = true;

			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
		}

		static int frameCounter = 0;
		if (gotAny && frameCounter % 60 == 0) {
			std::cout << "[CLIENT] Receiving game state (frame #" << outState.frameNumber << ")" << std::endl;
		}
		frameCounter++;

		return gotAny;
	}
	catch (const std::exception& e) {
		std::cerr << "[CLIENT EXCEPTION] receiveLatestGameState: " << e.what() << std::endl;
		return false;
	}
	catch (...) {
		std::cerr << "[CLIENT EXCEPTION] receiveLatestGameState: Unknown error" << std::endl;
		return false;
	}
}
