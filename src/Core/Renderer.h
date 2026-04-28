#pragma once

class Renderer {

public:

	Renderer() = default;

	void render(const Match& match);

	void renderMatch(sf::RenderWindow& window, const World& world);

	void renderHUD(int homeScore, int awayScore, float matchTimer, int stateID);

};
