#include <SFML/Graphics.hpp>
#include "Test.h"
#include "Core/GameEngine.h"
#include <iostream>

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
// Execution starts here.
// - In debug mode, runs networking and state machine tests.
// - Then launches the actual game (GameEngine).
// ============================================================================

int main()
{
	std::cout << "===================================" << std::endl;
	std::cout << "   SPCE - Starting Up...          " << std::endl;
	std::cout << "===================================" << std::endl;

#ifdef _DEBUG
	// ------------------------------------------------------------------------
	// DEBUG MODE: Run automated tests before starting the game.
	// ------------------------------------------------------------------------
	try {
		// SANTI: Networking handshake test (host <-> client exchange).
		Test::testNetworkingHandshakeStep3();

		// Visual state machine tests.
		Test::testEngineStateMachine();
		Test::testMatchStateMachine();

		std::cout << "\n=== All Tests PASSED ===" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Test failed: " << e.what() << std::endl;
		return 1;
	}
#endif

	// ------------------------------------------------------------------------
	// NORMAL GAME START (debug or release)
	// ------------------------------------------------------------------------
	std::cout << "\nStarting game window..." << std::endl;

	try {
		GameEngine engine;
		engine.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Game error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}