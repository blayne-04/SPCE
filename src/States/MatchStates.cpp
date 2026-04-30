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
	World& world = match.getWorld();
	Ball& ball = world.ball();

	// SANTI 28/04/2026: Kickoff is a real phase.
	// The match remains in KickoffState until the kicking team completes an opening pass.
	const int kickoffTeamSide = static_cast<int>(match.getKickoffTeamSide());
	const int kickoffOwnerId = (kickoffTeamSide == Config::AWAY_TEAM_SIDE) ? 4 : 0;

	// SANTI 28/04/2026: During kickoff, the defending team cannot cross midfield and
	// must stay outside the kickoff circle.
	//
	// We enforce this in TWO ways:
	// 1) Ignore defending-team inputs (so AI cannot "press" early).
	// 2) Clamp defending-team positions via World::enforceKickoffDefenderRestrictions().
	FrameInput sanitized = frameData;

	const bool kickoffIsHome = (kickoffTeamSide == Config::HOME_TEAM_SIDE);
	const int defStart = kickoffIsHome ? 4 : 0;
	const int defEndInclusive = kickoffIsHome ? 7 : 3;

	for (int id = defStart; id <= defEndInclusive; ++id) {
		// SANTI 28/04/2026: Allow movement during kickoff (players can reposition),
		// but block all ball-interaction actions until open play begins.
		sanitized.inputs[static_cast<std::size_t>(id)].shootDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].passDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].tackleDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].switchDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].lungeDown = false;
	}

	// SANTI 28/04/2026: Keep the kickoff taker stationary on the center spot
	// until the opening pass is taken. This avoids awkward "ball teleports" if the
	// player moves before kickoff.
	if (!mKickoffPassStarted) {
		sanitized.inputs[static_cast<std::size_t>(kickoffOwnerId)].moveDirection = sf::Vector2f(0.f, 0.f);
		sanitized.inputs[static_cast<std::size_t>(kickoffOwnerId)].shootDown = false;
		sanitized.inputs[static_cast<std::size_t>(kickoffOwnerId)].tackleDown = false;
		sanitized.inputs[static_cast<std::size_t>(kickoffOwnerId)].switchDown = false;
		sanitized.inputs[static_cast<std::size_t>(kickoffOwnerId)].lungeDown = false;
	}

	// Keep the ball attached to the kickoff owner until the pass begins.
	// World::resetKickoff(...) sets this up, but we guard against stale state.
	if (!mKickoffPassStarted) {
		if (ball.getOwner() != kickoffOwnerId) {
			ball.setOwner(kickoffOwnerId);
		}
		world.attachBallToOwnerIfAny();
	}

	// Allow player movement for the kicking team. Defenders are frozen by sanitation above.
	world.applyFrameMovement(sanitized, dt);
	PhysicsEngine::resolvePlayerSeparation(world, dt);

	// SANTI 28/04/2026: Always enforce that no one can stand inside the goal mouth.
	// This prevents players drifting into the net during restarts.
	world.enforceNoPlayersInsideGoalMouth();

	// Enforce real-football kickoff constraints on the defending team.
	world.enforceKickoffDefenderRestrictions(kickoffTeamSide);

	// Start the opening kickoff pass (only pass is allowed to begin play).
	if (!mKickoffPassStarted) {
		if (world.isPassPressed(sanitized, kickoffOwnerId)) {
			world.kickKickoffPassToTeammate(kickoffOwnerId);
			mKickoffPassStarted = true;
		}
	}

	// Advance the ball if it is loose (kickoff pass is guided travel with owner = -1).
	if (ball.getOwner() < 0) {
		ball.update(dt);
	}

	// Kickoff completes only when the kickoff pass resolves to a teammate owner.
	if (mKickoffPassStarted) {
		if (!ball.isGuidedInFlight() && ball.getOwner() >= 0) {
			mKickoffPassStarted = false;
			world.commitActionButtonHistory(sanitized);
			match.TransitionTo(std::make_unique<PlayingState>());
			return;
		}
	}

	// Maintain edge detection during kickoff so held buttons behave consistently.
	world.commitActionButtonHistory(sanitized);
}

void KickoffState::onEnter(Match& match) {
	// SANTI 28/04/2026: Reset kickoff runtime state.
	mKickoffPassStarted = false;

	// Reset world positions and ball ownership for the correct kicking side.
	match.getWorld().resetKickoff(static_cast<int>(match.getKickoffTeamSide()));
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

	// SANTI 28/04/2026: Prevent any players from drifting into the goal mouth.
	// This is enforced regardless of possession for football-sense.
	world.enforceNoPlayersInsideGoalMouth();


	// 2) Possession mechanics + ball stepping.
	//
	// Separation of concerns:
	// - World owns "how possession/pickup works" (attachBallToOwnerIfAny, tryPickupLooseBall).
	// - PlayingState owns "what inputs mean" (pass/shoot triggers) and match rules.
	Ball& ball = world.ball();
	// SANTI 29/04/26: IMPORTANT BUGFIX FOR STEALS/TACKLES.
	//
	// Ball::mStealCooldown is used as a generic "interaction cooldown":
	// - after kicks (pickup grace)
	// - after receiving/intercepting (possession protection)
	//
	// Ball::update(dt) is the only place that ticks this cooldown down each frame.
	// Previously we only called ball.update(dt) when the ball was loose (owner == -1).
	// That meant once the ball became owned again, the cooldown never decreased,
	// which effectively disabled tackles/steals forever (World::resolveTackleSteals
	// early-returns if ball.getStealCooldown() > 0).
	//
	// Fix: always call ball.update(dt) once per tick. When the ball is owned,
	// Ball::update only ticks cooldown and returns without moving the ball.
	ball.update(dt);

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

		// SANTI 28/04/2026: While a goalkeeper holds the ball, keep opponents out
		// of the goalkeeper's six-yard box so distribution stays playable.
		world.enforceGoalkeeperSixYardBoxProtection();

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
		// SANTI 28/04/2026: Real football kickoff rule.
		// The conceding team (Home) takes the next kickoff.
		match.setKickoffTeamSide(Config::HOME_TEAM_SIDE);
		match.TransitionTo(std::make_unique<KickoffState>());
		return;
	}

	if (world.awayGoal().checkBallCollision(ball)) {
		match.incrementScore(TEAMS::TEAM1); // Home scored in right/away goal.
		// SANTI 28/04/2026: Conceding team (Away) takes the next kickoff.
		match.setKickoffTeamSide(Config::AWAY_TEAM_SIDE);
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
