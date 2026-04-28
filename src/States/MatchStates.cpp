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

	World& world = match.getWorld(); // SANTI: local ref keeps code flat (never-nest).

	// 1) Apply authoritative per-player movement for this frame.
	// Movement is a simulation mechanic, so World owns it.
	world.applyFrameMovement(frameData, dt);

	// 2) Possession mechanics + ball stepping.
	//
	// Separation of concerns:
	// - World owns "how possession/pickup works" (attachBallToOwnerIfAny, tryPickupLooseBall).
	// - PlayingState owns "what inputs mean" (pass/shoot triggers) and match rules.
	Ball& ball = world.ball();
	const int ownerId = ball.getOwner();

	// If owned, keep ball attached to the owner, then allow owner inputs to kick.
	if (ownerId >= 0 && ownerId < static_cast<int>(Config::kNumPlayers)) {
		world.attachBallToOwnerIfAny();

		const InputPacket& ownerInput = frameData.inputs[ownerId];

		// MVP: DOWN-state triggers. Later you can compute edges on the host.
		if (ownerInput.passDown) {
			ball.applyPass();
		}
		else if (ownerInput.shootDown) {
			ball.applyShot();
		}
	}

	// If loose, integrate ball physics and then allow pickup/interceptions.
	if (ball.getOwner() < 0) {
		ball.update(dt);
		world.tryPickupLooseBall(Config::BALL_STEAL_RADIUS);
	}

	// 3) Goal detection and scoring.
	//
	// Goal object owns detection (checkBallCollision).
	// PlayingState owns the rule: "if goal, increment score and restart at kickoff".
	if (world.homeGoal().checkBallCollision(ball)) {
		match.incrementScore(TEAMS::TEAM2); // Away scored in left/home goal.
		match.TransitionTo(std::make_unique<KickoffState>());
		return;
	}

	if (world.awayGoal().checkBallCollision(ball)) {
		match.incrementScore(TEAMS::TEAM1); // Home scored in right/away goal.
		match.TransitionTo(std::make_unique<KickoffState>());
		return;
	}

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
