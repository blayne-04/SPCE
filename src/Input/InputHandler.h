#pragma once

#include "Network/NetworkManager.h"

class InputHandler {

public:


	InputHandler() = default;

	InputPacket getLocalInput(int slotID);


};
