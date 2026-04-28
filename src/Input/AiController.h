#pragma once

#include "../Simulation/Match.h"
#include "../Network/NetworkManager.h"

class AIController {

public:

	AIController() = default;

	InputPacket getAIInput(int playerID, const Match& match);


};
