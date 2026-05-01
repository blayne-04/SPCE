#pragma once

/**
 * @file GameEngine.h
 * @brief Main application loop and high-level shared services.
 */

#include <SFML/Graphics.hpp>
#include "../Simulation/Match.h"
#include "../Network/NetworkManager.h"
#include <vector>
#include <memory>

class EngineState;

/**
 * @class GameEngine
 * @brief Owns the window, state stack, Match, and NetworkManager.
 *
 * GameEngine does not contain soccer rules. It delegates to EngineState and
 * Match so the architecture stays separated.
 */
class GameEngine {
public:
	/** @brief Create the render window and initial game state. */
	GameEngine();

	/** @brief Destroy engine resources. */
	~GameEngine();

	/** @brief Run the main event/update/render loop. */
	void run();

	/** @brief Push a state on top of the current state stack. */
	void pushState(std::unique_ptr<EngineState> state);

	/** @brief Pop the current top state. */
	void popState();

	/** @brief Replace the current state stack with one new state. */
	void transitionTo(std::unique_ptr<EngineState> state);

	/**
	 * @brief Clear every state and start from one replacement state.
	 *
	 * This is different from transitionTo(). transitionTo() only replaces the
	 * top state, which is correct for normal menu navigation. Pause-menu exit
	 * needs to remove the paused match underneath too, so it uses this method.
	 */
	void resetStateStack(std::unique_ptr<EngineState> state);

	/**
	 * @brief Reset the shared Match object for a fresh game.
	 *
	 * GameEngine owns one Match service, so states call this before starting a
	 * new play mode or returning to the main menu from an ended/abandoned match.
	 */
	void resetMatch();

	/**
	 * @brief Stop the shared NetworkManager socket/session.
	 *
	 * Used with resetMatch() when leaving a match so the old LAN session does
	 * not keep a UDP port bound while the user is back at the main menu.
	 */
	void resetNetwork();

	/** @brief Access the shared SFML window. */
	sf::RenderWindow& getWindow() { return mWindow; }

	/** @brief Access the shared match/referee object. */
	Match& getMatch() { return mMatch; }

	/** @brief Access the shared network manager. */
	NetworkManager& getNetwork() { return mNetworkManager; }

private:
	enum class PendingStateAction {
		None,
		Push,
		Pop,
		Transition,
		ResetStack
	};

	sf::RenderWindow mWindow;
	std::vector<std::unique_ptr<EngineState>> mStates;
	Match mMatch;
	NetworkManager mNetworkManager;
	PendingStateAction mPendingStateAction = PendingStateAction::None;
	std::unique_ptr<EngineState> mPendingState;
	bool mIsTickingState = false;

	/* Handle OS Events e.g. (Close Window) */
	void processOsEvents();

	/**
	 * @brief Apply one deferred state-stack change after tick() returns.
	 *
	 * A state should not erase/replace the state stack while its own tick()
	 * function is still executing. Game Over buttons and pause exits request a
	 * transition during tick(), and GameEngine safely applies it afterward.
	 */
	void applyPendingStateAction();
};
