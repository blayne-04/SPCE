#include <SFML/Graphics.hpp>
#include "Test.h"
#include "Core/GameEngine.h"
#include <iostream>

int main()
{
    std::cout << "===================================" << std::endl;
    std::cout << "   SPCE - Starting Up...          " << std::endl;
    std::cout << "===================================" << std::endl;

#ifdef _DEBUG
    // Run tests only in debug mode
    try {
        Test::testEngineStateMachine();
        Test::testMatchStateMachine();
        std::cout << "\n=== All Tests PASSED ===" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
#endif

    // Start the game directly (no waiting for input)
    std::cout << "\nStarting game window..." << std::endl;
    
    try {
        GameEngine engine;
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "Game error: " << e.what() << std::endl;
        return 1;
    }

    return 0;   
}
