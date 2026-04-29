#include "MatchStates.h"
#include "../Simulation/Match.h"
#include "../Common/Constants.h"
#include "../Simulation/PhysicsEngine.h"

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

	// SANTI: Resolve overlaps after movement so players don't stack.
	PhysicsEngine::resolvePlayerSeparation(world, dt);


	// 2) Possession mechanics + ball stepping.
	//
	// Separation of concerns:
	// - World owns "how possession/pickup works" (attachBallToOwnerIfAny, tryPickupLooseBall).
	// - PlayingState owns "what inputs mean" (pass/shoot triggers) and match rules.
	Ball& ball = world.ball();
	int ownerId = ball.getOwner();

	// If owned, keep ball attached to the owner, then allow owner inputs to kick.
	if (ownerId >= 0 && ownerId < static_cast<int>(Config::kNumPlayers)) {
		world.attachBallToOwnerIfAny();

		// SANTI 28/04/2026: Resolve tackles before reading owner actions.
		// A successful tackle transfers possession immediately, so the old owner
		// should NOT also get to pass/shoot on the same tick.
		world.resolveTackleSteals(frameData, dt);
		ownerId = ball.getOwner();
		if (ownerId < 0) return;
		if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;

		// SANTI: Input is DOWN-state; World computes edges for pass/shoot.
		if (world.isPassPressed(frameData, ownerId)) {
			// SANTI: Keep PlayingState readable. World owns pass mechanics
			// (including interception query); PlayingState only interprets input.
			world.kickGuaranteedPassWithInterception(ownerId);
		}
		else if (world.isShootPressed(frameData, ownerId)) {
			// SANTI: Guaranteed shot (no mouse aim) with defender interception.
			world.kickGuaranteedShotWithInterception(ownerId);
		}
	}

	// If loose, integrate ball physics and then allow pickup/interceptions.
	if (ball.getOwner() < 0) {
		ball.update(dt);

		// SANTI 28/04/2026: During guided travel (guaranteed pass/shot), we do NOT
		// allow pickup. Possession is assigned only when the guided travel ends.
		// This matches your rule: "don't switch possession until success."
		if (!ball.isGuidedInFlight()) {
			world.tryPickupLooseBall(Config::BALL_STEAL_RADIUS);
		}
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

	// SANTI 28/04/2026: Commit action button DOWN-state history after reading edges.
	// This makes pass/shoot edge detection correct even when possession changes.
	world.commitActionButtonHistory(frameData);

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
