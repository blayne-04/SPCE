#pragma once

class Goal {
public:

	Goal() = default;

	bool checkBallCollision(const Ball& ball);

private:

	int mSide;

};
