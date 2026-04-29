#include "NetworkManager.h"
#include <iostream>
#include <queue>

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

		std::cout << "[HOST] Successfully bound to port " << localPort << std::endl;
		std::cout << "[HOST] Socket is non-blocking and ready to receive" << std::endl;
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
		std::cout << "[CLIENT] Connecting to host " << remoteAddress.toString() 
		          << ":" << remotePort << std::endl;

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

		std::cout << "[CLIENT] Successfully bound to local port " << mLocalPort << std::endl;
		std::cout << "[CLIENT] Ready to send/receive packets" << std::endl;
		std::cout << "[CLIENT] Host endpoint: " << mRemoteAddress.toString() 
		          << ":" << mRemotePort << std::endl;
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
		
		if (sendStatus != sf::Socket::Status::Done) {
			std::cerr << "[CLIENT WARNING] JOIN_REQUEST send failed. Status: " 
			          << static_cast<int>(sendStatus) << std::endl;
			std::cerr << "[CLIENT WARNING] Possible network issue or firewall blocking" << std::endl;
		} else {
			static int requestCount = 0;
			if (requestCount % 60 == 0) { // Log every 60 attempts (~1 second at 60fps)
				std::cout << "[CLIENT] Sending JOIN_REQUEST to " << mRemoteAddress.toString() 
				          << ":" << mRemotePort << " (attempt " << requestCount + 1 << ")" << std::endl;
			}
			requestCount++;
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

			// Validate sender
			if (!sender.has_value()) {
				std::cerr << "[CLIENT WARNING] Received packet with no sender address" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			if (*sender != mRemoteAddress || static_cast<uint16>(senderPort) != mRemotePort) {
				std::cerr << "[CLIENT WARNING] Received packet from unexpected source: " 
				          << sender->toString() << ":" << senderPort 
				          << " (expected " << mRemoteAddress.toString() << ":" << mRemotePort << ")" << std::endl;
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

			// Check if it's an ASSIGNMENT message
			if (messageType == static_cast<std::uint8_t>(NetMsg::ASSIGNMENT)) {
				std::uint8_t assignedId = 0;
				if (receivedPacket >> assignedId) {
					outId = assignedId;
					std::cout << "[CLIENT SUCCESS] Received player ID assignment: " 
					          << static_cast<int>(assignedId) << std::endl;
					return true;
				} else {
					std::cerr << "[CLIENT ERROR] Received ASSIGNMENT but failed to parse ID" << std::endl;
				}
			} else {
				std::cout << "[CLIENT INFO] Received message type " << static_cast<int>(messageType) 
				          << " while waiting for ASSIGNMENT" << std::endl;
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
// HOST SIDE: check for JOIN_REQUEST and send ASSIGNMENT response
// ----------------------------------------------------------------------------
void NetworkManager::handleHandshakeRequests()
{
	try {
		sf::Packet receivedPacket;
		std::optional<sf::IpAddress> sender;
		unsigned short senderPort = 0;

		auto receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);

		// Drain socket looking for JOIN_REQUEST messages
		while (receiveStatus != sf::Socket::Status::NotReady) {

			if (receiveStatus == sf::Socket::Status::Error) {
				std::cerr << "[HOST ERROR] Socket receive error in handleHandshakeRequests" << std::endl;
				return;
			}

			if (receiveStatus != sf::Socket::Status::Done) {
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			if (!sender.has_value()) {
				std::cerr << "[HOST WARNING] Received packet with no sender address" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Read message type
			std::uint8_t messageType = 0;
			if (!(receivedPacket >> messageType)) {
				std::cerr << "[HOST WARNING] Failed to read message type" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Check if it's a JOIN_REQUEST
			if (messageType == static_cast<std::uint8_t>(NetMsg::JOIN_REQUEST)) {
				const std::uint8_t assignedPlayerId = 1;

				// Learn/update the client's endpoint
				mRemoteAddress = *sender;
				mRemotePort = static_cast<uint16>(senderPort);

				std::cout << "[HOST] Received JOIN_REQUEST from " << sender->toString() 
				          << ":" << senderPort << std::endl;
				std::cout << "[HOST] Assigning player ID " << static_cast<int>(assignedPlayerId) << std::endl;

				// Send ASSIGNMENT response
				sf::Packet responsePacket;
				responsePacket << static_cast<std::uint8_t>(NetMsg::ASSIGNMENT) << assignedPlayerId;
				
				auto sendStatus = mSocket.send(responsePacket, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
				
				if (sendStatus != sf::Socket::Status::Done) {
					std::cerr << "[HOST ERROR] Failed to send ASSIGNMENT. Status: " 
					          << static_cast<int>(sendStatus) << std::endl;
				} else {
					std::cout << "[HOST SUCCESS] Sent ASSIGNMENT to client" << std::endl;
				}
			}

			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "[HOST EXCEPTION] handleHandshakeRequests: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[HOST EXCEPTION] handleHandshakeRequests: Unknown error" << std::endl;
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
					const std::uint8_t assignedPlayerId = 1;
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
			// Silently return - no client connected yet
			return;
		}

		sf::Packet packet;
		packet << static_cast<std::uint8_t>(NetMsg::STATE) << gameStatePacket;

		auto sendStatus = mSocket.send(packet, mRemoteAddress, static_cast<unsigned short>(mRemotePort));
		
		if (sendStatus != sf::Socket::Status::Done) {
			static int errorCount = 0;
			if (errorCount % 60 == 0) { // Log every 60 failures
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
				std::cout << "[CLIENT INFO] Received non-STATE message (type " 
				          << static_cast<int>(messageType) << ")" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Optional noise filter (ignore non-host senders)
			if (sender.has_value() && *sender != mRemoteAddress) {
				std::cerr << "[CLIENT WARNING] Ignoring STATE from non-host: " 
				          << sender->toString() << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}
			
			if (static_cast<uint16>(senderPort) != mRemotePort) {
				std::cerr << "[CLIENT WARNING] Ignoring STATE from wrong port: " 
				          << senderPort << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			GameStatePacket state{};
			if (!(receivedPacket >> state)) {
				std::cerr << "[CLIENT WARNING] Failed to deserialize GameStatePacket" << std::endl;
				receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
				continue;
			}

			// Keep newest snapshot
			if (!gotAny || state.frameNumber > outState.frameNumber) {
				outState = state;
			}
			gotAny = true;

			receiveStatus = mSocket.receive(receivedPacket, sender, senderPort);
		}

		// Log reception periodically
		static int frameCounter = 0;
		if (gotAny && frameCounter % 60 == 0) {
			std::cout << "[CLIENT] Received " << packetCount << " game state packet(s), frame #" 
			          << outState.frameNumber << std::endl;
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
