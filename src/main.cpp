/**
 * @file main.cpp
 * @brief Program entry point for Super Copa Peru Evolution.
 */

#include <SFML/Graphics.hpp>
#include "Core/GameEngine.h"
#include "Test.h"
#include <iostream>

 /**
  * @brief Program entry point.
  *
  * Creates the single GameEngine instance and runs the main loop until the user
  * closes the window or an exception occurs.
  */
int main()
{
	std::cout << "===================================" << std::endl;
	std::cout << "   SPCE - Starting Up...          " << std::endl;
	std::cout << "===================================" << std::endl;

	try {
#if defined(_DEBUG)
		Test::runTests();
#endif

		// Create ONE GameEngine instance for the entire program lifetime
		GameEngine engine;

		std::cout << "\nGame window created. Starting main loop..." << std::endl;

		// Run the game (window stays open until closed)
		engine.run();

		std::cout << "\nGame closed cleanly." << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Game error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
