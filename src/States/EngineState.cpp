#include "EngineState.h"
#include "../Core/GameEngine.h"

/* ========================================= */
/*               START MENU                  */
/* ========================================= */

StartMenuState::StartMenuState() 
{

}

void StartMenuState::tick(GameEngine& engine, float dt) 
{

}


void StartMenuState::render(sf::RenderWindow& window) 
{

}

/* ========================================= */
/*           SETTINGS MENU                   */
/* ========================================= */

void SettingsMenuState::tick(GameEngine& engine, float dt) 
{

}

void SettingsMenuState::render(sf::RenderWindow& window) 
{

}

/* ========================================= */
/*            PAUSE MENU                     */
/* ========================================= */

void PauseMenuState::tick(GameEngine& engine, float dt) 
{

}

void PauseMenuState::render(sf::RenderWindow& window)
{

}

/* ========================================= */
/*            CLIENT PLAY                    */
/* ========================================= */

void ClientPlayingState::tick(GameEngine& engine, float dt) 
{
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	InputPacket clientInput = mInputHandler.pollInput();
}

void ClientPlayingState::render(sf::RenderWindow& window) 
{

}

/* ========================================= */
/*                HOST PLAY                  */
/* ========================================= */

void HostPlayingState::tick(GameEngine& engine, float dt) 
{

}

void HostPlayingState::render(sf::RenderWindow& window) 
{

}

/* ========================================= */
/*             SINGLEPLAYER                  */
/* ========================================= */

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt) 
{

}

void SinglePlayerPlayingState::render(sf::RenderWindow& window) 
{

}