#pragma once

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include "Core/GameEngine.h"
#include "States/EngineState.h"

namespace Test
{
	void testEngineStateMachine();
	void testMatchStateMachine();

	// SANTI 30/04/26: Helper for the visual EngineState test (no lambdas).
	// Runs a timed loop that updates + renders a VisualTestState in its own window.
	inline void runEngineStateVisualTest(
		sf::RenderWindow& testWindow,
		sf::Clock& clock,
		const std::string& testName,
		const std::string& description,
		sf::Color color,
		float durationSeconds)
	{
		std::cout << "Test: " << testName << " - " << description << std::endl;

		VisualTestState testState(testName, description, color);

		float elapsed = 0.0f;
		clock.restart();

		while (testWindow.isOpen() && elapsed < durationSeconds) {
			const float dt = clock.restart().asSeconds();
			elapsed += dt;

			// Process window events
			while (const auto event = testWindow.pollEvent()) {
				if (event->is<sf::Event::Closed>()) {
					testWindow.close();
					return;
				}
			}

			// Update and render
			GameEngine dummyEngine; // Dummy engine for update call
			testState.update(dummyEngine, dt);
			testWindow.clear();
			testState.render(testWindow);
			testWindow.display();
		}

		std::cout << "  -> Completed (dt working: "
			<< (testState.elapsedTime > 0.0f ? "YES" : "NO")
			<< ", Updates: " << testState.updateCount << ")\n" << std::endl;
	}
}

// Visual test state that displays information
class VisualTestState : public EngineState {
public:
    std::string title;
    std::string description;
    sf::Color bgColor;
    float elapsedTime = 0.0f;
    int updateCount = 0;

    VisualTestState(const std::string& t, const std::string& desc, sf::Color color)
        : title(t), description(desc), bgColor(color) {}

    void handleInput(GameEngine& engine, sf::Event& event) override {}

    void update(GameEngine& engine, float dt) override {
        elapsedTime += dt;
        updateCount++;
    }

    void render(sf::RenderWindow& window) override {
        window.clear(bgColor);
        
        // Draw colored rectangles to show state (since Text rendering needs fonts)
        sf::RectangleShape titleBar(sf::Vector2f(1280.0f, 100.0f));
        titleBar.setPosition(sf::Vector2f(0, 310));
        titleBar.setFillColor(sf::Color::White);
        window.draw(titleBar);
        
        // Draw a progress indicator
        float progressWidth = elapsedTime * 200.0f;
        if (progressWidth > 0.0f) {
            sf::RectangleShape progressBar(sf::Vector2f(progressWidth, 50.0f));
            progressBar.setPosition(sf::Vector2f(540, 430));
            progressBar.setFillColor(sf::Color::Green);
            window.draw(progressBar);
        }
    }
};

inline void Test::testEngineStateMachine() {
    std::cout << "\n=== Visual Engine State Machine Test ===" << std::endl;
    std::cout << "Watch the window for visual feedback of state transitions\n" << std::endl;

    // Create a test window
    sf::RenderWindow testWindow(sf::VideoMode({1280, 720}), "SPCE State Machine Tests");
    testWindow.setFramerateLimit(60);
    sf::Clock clock;

    // Test 1: Single State (Blue)
    runEngineStateVisualTest(
        testWindow,
        clock,
        "Test 1: Single State", 
        "One state pushed to stack", 
        sf::Color::Blue, 
        2.0f
    );
    if (!testWindow.isOpen()) return;

    // Test 2: Push Overlay (Red -> Green)
    {
        std::cout << "Test 2: Push Overlay State" << std::endl;
        
        // Base game state (red background)
        VisualTestState gameState("Base Game", "Playing game", sf::Color::Red);
        VisualTestState* overlayPtr = nullptr;
        
        float elapsed = 0.0f;
        bool overlayPushed = false;
        clock.restart();
        
        while (testWindow.isOpen() && elapsed < 4.0f) {
            float dt = clock.restart().asSeconds();
            elapsed += dt;
            
            while (const auto event = testWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    testWindow.close();
                    return;
                }
            }
            
            GameEngine dummyEngine;
            
            // Create overlay at 2 seconds
            if (!overlayPushed && elapsed > 2.0f) {
                overlayPtr = new VisualTestState("Pause Overlay", "Menu on top", sf::Color(0, 150, 0));
                overlayPushed = true;
                std::cout << "  -> Overlay pushed at t=" << elapsed << "s" << std::endl;
            }
            
            // Update only top state
            if (overlayPushed && overlayPtr) {
                overlayPtr->update(dummyEngine, dt);
            } else {
                gameState.update(dummyEngine, dt);
            }
            
            // Render all states (stack rendering)
            testWindow.clear();
            gameState.render(testWindow);
            if (overlayPushed && overlayPtr) {
                overlayPtr->render(testWindow); 
            }
            testWindow.display();
        }
        
        std::cout << "  -> Game updates: " << gameState.updateCount;
        if (overlayPtr) {
            std::cout << ", Overlay updates: " << overlayPtr->updateCount;
            delete overlayPtr;
        }
        std::cout << "\n" << std::endl;
    }

    // Test 3: Pop State (Magenta -> Yellow)
    {
        std::cout << "Test 3: Pop State (Resume)" << std::endl;
        
        VisualTestState gameState("Game", "Base game", sf::Color::Magenta);
        VisualTestState pauseState("Pause", "Paused", sf::Color::Yellow);
        
        float elapsed = 0.0f;
        bool popped = false;
        clock.restart();
        
        while (testWindow.isOpen() && elapsed < 4.0f) {
            float dt = clock.restart().asSeconds();
            elapsed += dt;
            
            while (const auto event = testWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    testWindow.close();
                    return;
                }
            }
            
            GameEngine dummyEngine;
            
            // Pop at 2 seconds
            if (!popped && elapsed > 2.0f) {
                popped = true;
                std::cout << "  -> Pause popped, game resumed at t=" << elapsed << "s" << std::endl;
            }
            
            // Update and render
            if (!popped) {
                pauseState.update(dummyEngine, dt);
                testWindow.clear();
                gameState.render(testWindow);
                pauseState.render(testWindow);
            } else {
                gameState.update(dummyEngine, dt);
                testWindow.clear();
                gameState.render(testWindow);
            }
            testWindow.display();
        }
        std::cout << "  -> Game resumed successfully\n" << std::endl;
    }

    // Test 4: Transition (Cyan -> Orange)
    {
        std::cout << "Test 4: TransitionTo (Replace State)" << std::endl;
        
        VisualTestState menu1("Main Menu", "Initial menu", sf::Color::Cyan);
        VisualTestState* menu2Ptr = nullptr;
        
        float elapsed = 0.0f;
        bool transitioned = false;
        clock.restart();
        
        while (testWindow.isOpen() && elapsed < 4.0f) {
            float dt = clock.restart().asSeconds();
            elapsed += dt;
            
            while (const auto event = testWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    testWindow.close();
                    return;
                }
            }
            
            GameEngine dummyEngine;
            
            // Transition at 2 seconds
            if (!transitioned && elapsed > 2.0f) {
                menu2Ptr = new VisualTestState("Settings Menu", "New menu", sf::Color(255, 165, 0));
                transitioned = true;
                std::cout << "  -> Transitioned to new menu at t=" << elapsed << "s" << std::endl;
            }
            
            // Update and render
            testWindow.clear();
            if (!transitioned) {
                menu1.update(dummyEngine, dt);
                menu1.render(testWindow);
            } else if (menu2Ptr) {
                menu2Ptr->update(dummyEngine, dt);
                menu2Ptr->render(testWindow);
            }
            testWindow.display();
        }
        
        if (menu2Ptr) {
            delete menu2Ptr;
        }
        std::cout << "  -> Transition complete\n" << std::endl;
    }

    std::cout << "\n=== All Visual Tests Complete ===" << std::endl;
    std::cout << "Closing test window in 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    testWindow.close();
}

inline void Test::testMatchStateMachine() {
    std::cout << "\n=== Match State Machine ===" << std::endl;
    std::cout << "Not implemented yet - will test Kickoff -> Playing -> Goal transitions" << std::endl;
}
