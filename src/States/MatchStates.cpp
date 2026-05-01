#include "MatchStates.h"

/**
 * @file MatchStates.cpp
 * @brief Kickoff, playing, and game-over rule implementation.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help re-order/comment the
 * `PlayingState::update` pipeline for readability (movement, ball, goals, timer), without
 * changing the match rules. The team owned the match design and validated behavior through
 * play-testing.
 *
 * Example prompt used:
 * "Given these MatchState classes, suggest a readable `PlayingState::update` ordering for:
 * movement, ball update, goal detection, and match timer. Keep behavior unchanged; focus on
 * clarity and maintainability."
 */

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

	// Kickoff is a real phase: the match remains in KickoffState until the kicking
	// team starts the opening pass.
	const int kickoffTeamSide = static_cast<int>(match.getKickoffTeamSide());
	const int kickoffOwnerId = (kickoffTeamSide == Config::AWAY_TEAM_SIDE) ? 4 : 0;

	// During kickoff, the defending team cannot cross midfield and must stay outside
	// the kickoff circle.
	//
	// We enforce this in TWO ways:
	// 1) Ignore defending-team inputs (so AI cannot "press" early).
	// 2) Clamp defending-team positions via World::enforceKickoffDefenderRestrictions().
	FrameInput sanitized = frameData;

	const bool kickoffIsHome = (kickoffTeamSide == Config::HOME_TEAM_SIDE);
	const int defStart = kickoffIsHome ? 4 : 0;
	const int defEndInclusive = kickoffIsHome ? 7 : 3;

	for (int id = defStart; id <= defEndInclusive; ++id) {
		// Allow movement during kickoff (players can reposition),
		// but block all ball-interaction actions until open play begins.
		sanitized.inputs[static_cast<std::size_t>(id)].shootDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].passDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].tackleDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].switchDown = false;
		sanitized.inputs[static_cast<std::size_t>(id)].lungeDown = false;
	}

	// Keep the kickoff taker stationary on the center spot
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

	// Always enforce that no one can stand inside the goal mouth.
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
	// Reset kickoff runtime state.
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


	// Soccer clock counts up only while playing.
	match.addTimeSec(dt);

	World& world = match.getWorld();

	// Update cow event before moving players so collisions use current cow positions.
	world.updateCows(dt);

	// 1) Apply authoritative per-player movement for this frame.
	// Movement is a simulation mechanic, so World owns it.
	world.applyFrameMovement(frameData, dt);

	// Resolve overlaps after movement so players don't stack.
	PhysicsEngine::resolvePlayerSeparation(world, dt);

	// Cows are solid obstacles: push players out of cows after movement/separation.
	world.resolveCowPlayerCollisions(dt);

	// Prevent any players from drifting into the goal mouth.
	// This is enforced regardless of possession for football-sense.
	world.enforceNoPlayersInsideGoalMouth();


	// 2) Possession mechanics + ball stepping.
	//
	// Separation of concerns:
	// - World owns "how possession/pickup works" (attachBallToOwnerIfAny, tryPickupLooseBall).
	// - PlayingState owns "what inputs mean" (pass/shoot triggers) and match rules.
	Ball& ball = world.ball();
	// IMPORTANT: Ball interaction cooldowns.
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
	const sf::Vector2f ballPrevPos = ball.getPosition();
	ball.update(dt);

	// Cows can block the ball even during guided pass/shot travel. This may cancel
	// guided flight and convert the ball back to velocity-based motion.
	world.resolveCowBallCollisions(dt, ballPrevPos);

	int ownerId = ball.getOwner();

	// If owned, keep ball attached to the owner, then allow owner inputs to kick.
	if (ownerId >= 0 && ownerId < static_cast<int>(Config::kNumPlayers)) {
		world.attachBallToOwnerIfAny();

		// Resolve tackles before reading owner actions.
		// A successful tackle transfers possession immediately, so the old owner
		// should NOT also get to pass/shoot on the same tick.
		world.resolveTackleSteals(frameData, dt);
		ownerId = ball.getOwner();
		if (ownerId < 0) return;
		if (ownerId >= static_cast<int>(Config::kNumPlayers)) return;

		// While a goalkeeper holds the ball, keep opponents out
		// of the goalkeeper's six-yard box so distribution stays playable.
		world.enforceGoalkeeperSixYardBoxProtection();

		// Input is DOWN-state; World computes edges for pass/shoot.
		if (world.isPassPressed(frameData, ownerId)) {
			// Keep PlayingState readable. World owns pass mechanics
			// (including interception query); PlayingState only interprets input.
			world.kickGuaranteedPassWithInterception(ownerId);
		}
		else if (world.isShootPressed(frameData, ownerId)) {
			// Guaranteed shot (no mouse aim) with defender interception.
			world.kickGuaranteedShotWithInterception(ownerId);
		}
	}

	// If loose, integrate ball physics and then allow pickup/interceptions.
	if (ball.getOwner() < 0) {
		// During guided travel (guaranteed pass/shot), we do NOT
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
		// Real football kickoff rule: the conceding team (Home) takes the next kickoff.
		match.setKickoffTeamSide(Config::HOME_TEAM_SIDE);
		match.TransitionTo(std::make_unique<KickoffState>());
		return;
	}

	if (world.awayGoal().checkBallCollision(ball)) {
		match.incrementScore(TEAMS::TEAM1); // Home scored in right/away goal.
		// Conceding team (Away) takes the next kickoff.
		match.setKickoffTeamSide(Config::AWAY_TEAM_SIDE);
		match.TransitionTo(std::make_unique<KickoffState>());
		return;
	}

	// Commit action button DOWN-state history after reading edges.
	// This makes pass/shoot edge detection correct even when possession changes.
	world.commitActionButtonHistory(frameData);

	// End match when time limit reached.
	if (match.getTimerSec() < Config::MATCH_DURATION_SECONDS) return;
	match.TransitionTo(std::make_unique<GameOverState>());
}

void PlayingState::onEnter(Match& match)
{
}

void PlayingState::onExit(Match& match)
{
}
