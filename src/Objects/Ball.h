#pragma once

#include "SFML/Graphics.hpp"
#include "./Common/Constants.h"

class Ball : public sf::CircleShape
{
public:	
	/* Defaults: mOwnerID : -1, mStealCooldown : 0, Radius : 0.5f, Color : White */
	Ball() : mOwnerID(-1), mStealCooldown(0.f) {
		setRadius(BALL_RADIUS);
		setFillColor(sf::Color::White);
		setOrigin(sf::Vector2f(getRadius(),getRadius()));
	}

	~Ball() = default;


	/* --- POSSESSION --- */
	/* Sets the owner of the ball (int playerID) */
	void setOwner(const int playerID) { mOwnerID = playerID; }
	/* Gets the owner of the ball */
	int getOwner() const { return mOwnerID; }
	/* Clear the owner of the ball (sets OwnerID to -1) */
	void clearOwner() { mOwnerID = -1; }

	/* --- COOLDOWNS --- */
	/* Gets the steal cooldown of the ball */
	float getStealCooldown() const { return mStealCooldown; }
	/* Sets the steal cooldown of the ball (const float cooldown) */
	void setStealCooldown(const float cooldown) { mStealCooldown = cooldown; }

	/* --- PHYSICS & ACTIONS --- */
	void applyKick(sf::Vector2f direction);
	void applyPass();
	void applyShot();

	void update(const float deltaTiem);
	
	
private:
	float mStealCooldown;
	int mOwnerID;
	sf::Vector2f mVelocity;
};