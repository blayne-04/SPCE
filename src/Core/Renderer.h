#pragma once

class Renderer {

public:

	Renderer() = default;

	void render(const Match& match);

	void renderHUD(int homeScore, int awayScore, float matchTimer, int stateID);

};
