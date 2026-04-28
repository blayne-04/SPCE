#pragma once

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <queue>                     // SANTI: used by testNetworkingHandshakeStep3()
#include "Network/NetworkManager.h"
#include "Common/Packets.h"
#include "Core/GameEngine.h"
#include "States/EngineState.h"

// SANTI: Tests reference Config::WINDOW_WIDTH/HEIGHT and Config::HOST_PORT.
#include "Common/Constants.h"

// ============================================================================
// TEST NAMESPACE
// ============================================================================
// Collection of smoke tests for state machines and networking handshake.
// Each test can be called independently from main() or a test runner.
// ============================================================================

namespace Test {

	void testNetworkingHandshakeStep3();
	void testEngineStateMachine();
	void testMatchStateMachine();

} // namespace Test

// ============================================================================
// VISUAL TEST STATE
// ============================================================================
// A simple EngineState that displays colored rectangles and a progress bar.
// Used to visually verify state transitions (push, pop, replace) without
// requiring fonts or full game assets.
// ============================================================================

class VisualTestState : public EngineState {
public:
	std::string title;
	std::string description;
	sf::Color bgColor;
	float elapsedTime = 0.0f;
	int updateCount = 0;

	VisualTestState(const std::string& t, const std::string& desc, sf::Color color)
		: title(t), description(desc), bgColor(color) {
	}

	void handleInput(GameEngine& engine, sf::Event& event) override {
		// No input handling for visual tests.
	}

	void update(GameEngine& engine, float dt) override {
		elapsedTime += dt;
		updateCount++;
	}

	void render(sf::RenderWindow& window) override {
		window.clear(bgColor);

		// Draw a white title bar (simulates UI without fonts).
		sf::RectangleShape titleBar(sf::Vector2f(1280.0f, 100.0f));
		titleBar.setPosition(sf::Vector2f(0, 310));
		titleBar.setFillColor(sf::Color::White);
		window.draw(titleBar);

		// Draw a green progress bar that grows with elapsedTime.
		float progressWidth = elapsedTime * 200.0f;
		if (progressWidth > 0.0f) {
			sf::RectangleShape progressBar(sf::Vector2f(progressWidth, 50.0f));
			progressBar.setPosition(sf::Vector2f(540, 430));
			progressBar.setFillColor(sf::Color::Green);
			window.draw(progressBar);
		}
	}
};

// ============================================================================
// TEST: ENGINE STATE MACHINE (VISUAL)
// ============================================================================
// Runs four visual sub-tests:
//   1) Single state          - blue background, progress bar.
//   2) Push overlay          - red game -> green pause overlay after 2s.
//   3) Pop state (resume)    - magenta game + yellow pause -> pop pause.
//   4) Transition (replace)  - cyan menu -> orange settings after 2s.
//
// The window shows coloured bars and progress to confirm dt works.
// ============================================================================

inline void Test::testEngineStateMachine() {
	std::cout << "\n=== Visual Engine State Machine Test ===" << std::endl;
	std::cout << "Watch the window for visual feedback of state transitions\n" << std::endl;

	// Create test window.
	sf::RenderWindow testWindow(sf::VideoMode({ 1280, 720 }), "SPCE State Machine Tests");
	testWindow.setFramerateLimit(60);
	sf::Clock clock;

	// Helper lambda: run a single visual state for a given duration.
	auto runVisualTest = [&](const std::string& testName, const std::string& description,
		sf::Color color, float durationSeconds) {
			std::cout << "Test: " << testName << " - " << description << std::endl;

			VisualTestState testState(testName, description, color);

			float elapsed = 0.0f;
			clock.restart();

			while (testWindow.isOpen() && elapsed < durationSeconds) {
				float dt = clock.restart().asSeconds();
				elapsed += dt;

				// Handle window close event.
				while (const auto event = testWindow.pollEvent()) {
					if (event->is<sf::Event::Closed>()) {
						testWindow.close();
						return;
					}
				}

				// Update and render.
				GameEngine dummyEngine; // Dummy engine for update call.
				testState.update(dummyEngine, dt);
				testWindow.clear();
				testState.render(testWindow);
				testWindow.display();
			}

			std::cout << "  -> Completed (dt working: "
				<< (testState.elapsedTime > 0.0f ? "YES" : "NO")
				<< ", Updates: " << testState.updateCount << ")\n" << std::endl;
		};

	// ------------------------------------------------------------------------
	// Test 1: Single State (Blue)
	// ------------------------------------------------------------------------
	runVisualTest(
		"Test 1: Single State",
		"One state pushed to stack",
		sf::Color::Blue,
		2.0f
	);

	// ------------------------------------------------------------------------
	// Test 2: Push Overlay (Red -> Green)
	// ------------------------------------------------------------------------
	{
		std::cout << "Test 2: Push Overlay State" << std::endl;

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

			// Push overlay after 2 seconds.
			if (!overlayPushed && elapsed > 2.0f) {
				overlayPtr = new VisualTestState("Pause Overlay", "Menu on top",
					sf::Color(0, 150, 0));
				overlayPushed = true;
				std::cout << "  -> Overlay pushed at t=" << elapsed << "s" << std::endl;
			}

			// Update only the top state.
			if (overlayPushed && overlayPtr) {
				overlayPtr->update(dummyEngine, dt);
			}
			else {
				gameState.update(dummyEngine, dt);
			}

			// Render all states (stack rendering: game first, then overlay).
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

	// ------------------------------------------------------------------------
	// Test 3: Pop State (Resume: Magenta -> Yellow)
	// ------------------------------------------------------------------------
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

			// Pop pause after 2 seconds.
			if (!popped && elapsed > 2.0f) {
				popped = true;
				std::cout << "  -> Pause popped, game resumed at t=" << elapsed << "s" << std::endl;
			}

			// Update and render.
			if (!popped) {
				pauseState.update(dummyEngine, dt);
				testWindow.clear();
				gameState.render(testWindow);
				pauseState.render(testWindow);
			}
			else {
				gameState.update(dummyEngine, dt);
				testWindow.clear();
				gameState.render(testWindow);
			}
			testWindow.display();
		}
		std::cout << "  -> Game resumed successfully\n" << std::endl;
	}

	// ------------------------------------------------------------------------
	// Test 4: Transition (Replace State: Cyan -> Orange)
	// ------------------------------------------------------------------------
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

			// Transition to new menu after 2 seconds.
			if (!transitioned && elapsed > 2.0f) {
				menu2Ptr = new VisualTestState("Settings Menu", "New menu",
					sf::Color(255, 165, 0)); // Orange
				transitioned = true;
				std::cout << "  -> Transitioned to new menu at t=" << elapsed << "s" << std::endl;
			}

			// Update and render.
			testWindow.clear();
			if (!transitioned) {
				menu1.update(dummyEngine, dt);
				menu1.render(testWindow);
			}
			else if (menu2Ptr) {
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

// ============================================================================
// TEST: MATCH STATE MACHINE (PLACEHOLDER)
// ============================================================================
// Intended to test Kickoff -> Playing -> Goal -> Reset transitions.
// Currently not implemented.
// ============================================================================

inline void Test::testMatchStateMachine() {
	std::cout << "\n=== Match State Machine ===" << std::endl;
	std::cout << "Not implemented yet - will test Kickoff -> Playing -> Goal transitions"
		<< std::endl;
}

// ============================================================================
// TEST: NETWORKING HANDSHAKE (STEP 3)
// ============================================================================
// SANTI: Smoke test that verifies host and client can exchange a single
//        InputPacket and one GameStatePacket.
//
// How it works:
//   1) Host binds to a test port (Config::HOST_PORT + 1).
//   2) Client connects to localhost on that port.
//   3) Client sends a neutral InputPacket (playerId = 4) repeatedly.
//   4) Host polls inputs - this teaches the host the client's UDP endpoint.
//   5) Host sends a GameStatePacket back to the learned client.
//   6) Client drains snapshots and keeps the newest.
//
// Asserts that both directions succeeded within a 2-second deadline.
// ============================================================================

inline void Test::testNetworkingHandshakeStep3()
{
	std::cout << "\n=== Networking Step 3 Smoke Test (Host <-> Client) ===\n";

	// Use a port unlikely to collide with your normal run.
	const unsigned short port = static_cast<unsigned short>(Config::HOST_PORT + 1);

	NetworkManager host;
	host.startHost(port);

	NetworkManager client;
	client.startClient(sf::IpAddress::LocalHost, port);

	std::queue<InputPacket> hostInbox;
	GameStatePacket clientState{};

	bool hostGotInput = false;
	bool clientGotState = false;

	std::uint32_t seq = 0;
	std::uint32_t frame = 0;

	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);

	while (std::chrono::steady_clock::now() < deadline)
	{
		// 1) Client sends handshake INPUT (neutral input is fine).
		InputPacket in{};
		in.playerId = 4;           // fallback away player id
		in.inputSequence = seq++;
		client.sendPlayerInput(in);

		// 2) Host polls inputs (this is where host learns client ip:port).
		host.pollIncomingInputs(hostInbox);
		if (!hostInbox.empty()) {
			hostGotInput = true;
		}

		// 3) Host sends a snapshot (will only work after host learned endpoint).
		GameStatePacket out{};
		out.frameNumber = frame++;
		out.pitchBounds = sf::FloatRect(
			sf::Vector2f(0.f, 0.f),
			sf::Vector2f(
				static_cast<float>(Config::WINDOW_WIDTH),
				static_cast<float>(Config::WINDOW_HEIGHT)
			)
		);
		host.sendGameState(out);

		// 4) Client drains snapshots and keeps newest.
		if (client.receiveLatestGameState(clientState)) {
			clientGotState = true;
		}

		if (hostGotInput && clientGotState) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	assert(hostGotInput && "Host did not receive any INPUT. Port mismatch or pollIncomingInputs broken.");
	assert(clientGotState && "Client did not receive any STATE. Host did not learn endpoint or sendGameState broken.");

	std::cout << "PASS: Host received INPUT and client received STATE.\n";
}
