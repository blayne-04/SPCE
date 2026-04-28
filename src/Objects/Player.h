#pragma once

#include "SFML/Graphics.hpp"

class Player : public sf::CircleShape {

public:

	Player() = default;

	void update(float downTime);

	void kickBall();


private:

	int mPlayerID;
	int mTeam;

};
