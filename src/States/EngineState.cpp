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


void StartMenuState::render(GameEngine& engine) 
{

}

/* ========================================= */
/*           SETTINGS MENU                   */
/* ========================================= */

void SettingsMenuState::tick(GameEngine& engine, float dt) 
{

}

void SettingsMenuState::render(GameEngine& engine) 
{

}

/* ========================================= */
/*            PAUSE MENU                     */
/* ========================================= */

void PauseMenuState::tick(GameEngine& engine, float dt) 
{

}

void PauseMenuState::render(GameEngine& engine)
{

}

/* ========================================= */
/*            CLIENT PLAY                    */
/* ========================================= */

void ClientPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		engine.pushState(std::make_unique<PauseMenuState>());
	}

	if (mMyPlayerID == 255) {
		network.sendJoinRequest();

		if (!network.pollIdAssignment(mMyPlayerID)) {
			return;
		}

		std::cout << "Assigned Player ID: " << static_cast<int>(mMyPlayerID) << std::endl;
	}

	InputPacket clientInput = mInputHandler.pollInput(mMyPlayerID);
	network.sendPlayerInput(clientInput);

	GameStatePacket latestGameState;
	if (network.receiveLatestGameState(latestGameState)) {
		engine.getMatch().overwriteWorld(latestGameState);
	}
}

void ClientPlayingState::render(GameEngine& engine) 
{
	mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
}

/* ========================================= */
/*                HOST PLAY                  */
/* ========================================= */

void HostPlayingState::tick(GameEngine& engine, float dt) 
{
	auto& network = engine.getNetwork();
	auto& match = engine.getMatch();

    /* Intake host input in slot 0 */
	FrameInput frameData;
	frameData.inputs[0] = mInputHandler.pollInput(HOST_ID);

    /* Track human player slots */
    bool isHuman[10] = { false };
    isHuman[0] = true;
    
    /* Collect client packets */
    std::queue<InputPacket> remotePackets;
    network.pollIncomingInputs(remotePackets);

    /* While there's incoming packets, process them */
    while (!remotePackets.empty()) {
        InputPacket p = remotePackets.front();
        remotePackets.pop();

		if (p.playerId >= 10) { 
            std::cout << "Received packet with invalid player ID: " << static_cast<int>(p.playerId) << std::endl;
            continue;
        }

        frameData.inputs[p.playerId] = p;
        isHuman[p.playerId] = true;
    }

    /* Fill gaps with packets from AI */
    for (uint8_t i = 0; i < 10; ++i) {
        if (!isHuman[i]) {
            /* Pass Id and match to each AI input call */
            frameData.inputs[i] = mAiController.getAIInput(i, match.getWorld());
        }
    }

    /* Call the update for match */
    match.update(frameData, dt);

    /* Send update across the network to all clients */
    network.sendGameState(match.getStatePacket());
}

void HostPlayingState::render(GameEngine& engine) 
{
	mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
}

/* ========================================= */
/*             SINGLEPLAYER                  */
/* ========================================= */

void SinglePlayerPlayingState::tick(GameEngine& engine, float dt) 
{
    	auto& match = engine.getMatch();

		FrameInput frameData;
        frameData.inputs[0] = mInputHandler.pollInput(HOST_ID);

        for(uint8_t i = 1; i < 10; ++i) {
            frameData.inputs[i] = mAiController.getAIInput(i, match.getWorld());
		}

        match.update(frameData, dt);
}

void SinglePlayerPlayingState::render(GameEngine& engine) 
{
    mRenderer.renderMatch(engine.getWindow(), engine.getMatch().getWorld(), mMyPlayerID);
}