#include "MatchStates.h"
#include "../Simulation/Match.h"
#include "../Common/Constants.h"

/* ========================================= */
/*              GAME OVER                    */
/* ========================================= */

GameOverState::GameOverState()
{
}

void GameOverState::update(Match& match, const FrameInput& frameData, float dt)
{
}

void GameOverState::onEnter(Match& match)
{
}

void GameOverState::onExit(Match& match)
{
}




/* ========================================= */
/*                KICKOFF                    */
/* ========================================= */

KickoffState::KickoffState()
{

}

void KickoffState::update(Match& match, const FrameInput& frameData, float dt) {
	match.TransitionTo(std::make_unique<PlayingState>());
}

void KickoffState::onEnter(Match& match) {
	match.getWorld().resetKickoff();
}

void KickoffState::onExit(Match& match)
{
}




/* ========================================= */
/*                PLAYING                    */
/* ========================================= */

PlayingState::PlayingState()
{
}

void PlayingState::update(Match& match, const FrameInput& frameData, float dt) {


	// SANTI: soccer clock counts UP only while playing.
	match.addTimeSec(dt);

	// SANTI: apply authoritative per-player movement for this frame.
	match.getWorld().applyFrameMovement(frameData, dt);

	// SANTI: end match when time limit reached.
	if (match.getTimerSec() < Config::MATCH_DURATION_SECONDS) return;
	match.TransitionTo(std::make_unique<GameOverState>());

	// SANTI: match timer counts up (soccer clock).
	// You'll need Match to expose a method OR make mMatchTimerSec mutable via a setter.
	// Example (preferred): match.addTime(dt);

	// MVP: apply frameData.inputs[i].moveDirection to move players in World.
	// (Either do it here, or call Player::applyInput(...) if you add it.)

	// End condition example:
	// if (match.getTimerSec() >= Config::MATCH_DURATION_SECONDS)
	/*{ match.TransitionTo(std::make_unique<GameOverState>()); }*/
}

void PlayingState::onEnter(Match& match)
{
}

void PlayingState::onExit(Match& match)
{
}
