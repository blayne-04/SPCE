/**
 * @file MenuInputHandler.cpp
 * @brief Placeholder menu event routing implementation.
 */

#include "MenuInputHandler.h"

/**
 * @brief Dispatch one SFML event to the specific menu-input helper.
 */
void MenuInputHandler::handleEvent(const sf::Event& event, GameEngine& engine, StartMenuState& state) {
	const auto* keyPressedEvent = event.getIf<sf::Event::KeyPressed>();
	if (keyPressedEvent) {
		onKeyPressed(*keyPressedEvent, engine, state);
		return;
	}

	const auto* mouseMovedEvent = event.getIf<sf::Event::MouseMoved>();
	if (mouseMovedEvent) {
		onMouseMoved(*mouseMovedEvent, state);
		return;
	}

	const auto* mouseClickedEvent = event.getIf<sf::Event::MouseButtonPressed>();
	if (mouseClickedEvent) {
		onMouseClicked(*mouseClickedEvent, engine, state);
		return;
	}
}

/**
 * @brief Handle keyboard events for menu navigation.
 */
void MenuInputHandler::onKeyPressed(const sf::Event::KeyPressed& key, GameEngine& engine, StartMenuState& state) {
	// Placeholder for keyboard menu navigation.
	// Intended behavior: arrow keys move selection, Enter selects, Escape backs out.
}

/**
 * @brief Handle mouse movement for menu hover behavior.
 */
void MenuInputHandler::onMouseMoved(const sf::Event::MouseMoved& mouse, StartMenuState& state) {
	// Placeholder for hover behavior.
	// Intended behavior: update which button the mouse is currently over.
}

/**
 * @brief Handle mouse clicks for menu selection.
 */
void MenuInputHandler::onMouseClicked(const sf::Event::MouseButtonPressed& mouse, GameEngine& engine, StartMenuState& state) {
	// Placeholder for menu click behavior.
	// Intended behavior: compare click position against button bounds and trigger the selected action.
}

/**
 * @brief Return true when a mouse position lies within a button rectangle.
 */
bool MenuInputHandler::isMouseOver(
	const sf::Vector2f& mousePosition,
	const sf::Vector2f& buttonPosition,
	const sf::Vector2f& buttonSize)
{
	const bool insideHorizontalBounds =
		mousePosition.x >= buttonPosition.x &&
		mousePosition.x <= buttonPosition.x + buttonSize.x;

	const bool insideVerticalBounds =
		mousePosition.y >= buttonPosition.y &&
		mousePosition.y <= buttonPosition.y + buttonSize.y;

	return insideHorizontalBounds && insideVerticalBounds;
}
