#pragma once

/**
 * @file MenuInputHandler.h
 * @brief Helper for menu input events.
 */

#include <SFML/Graphics.hpp>

// Forward declarations
class GameEngine;
class StartMenuState;

/**
 * @class MenuInputHandler
 * @brief Routes SFML menu events into menu actions.
 *
 * This class is intentionally small and does not own the menu state.
 */
class MenuInputHandler {
public:
	/** @brief Process one menu event. */
	void handleEvent(const sf::Event& event, GameEngine& engine, StartMenuState& state);

private:
	/** @brief Handle keyboard input for menu navigation. */
	void onKeyPressed(const sf::Event::KeyPressed& key, GameEngine& engine, StartMenuState& state);

	/** @brief Handle mouse movement for hover behavior. */
	void onMouseMoved(const sf::Event::MouseMoved& mouse, StartMenuState& state);

	/** @brief Handle mouse click selection. */
	void onMouseClicked(const sf::Event::MouseButtonPressed& mouse, GameEngine& engine, StartMenuState& state);

	/** @brief Check whether a mouse position is inside a rectangular button. */
	bool isMouseOver(const sf::Vector2f& mousePos, const sf::Vector2f& buttonPos, const sf::Vector2f& buttonSize);
};
