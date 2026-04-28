#include "World.h"
#include "../Common/Constants.h"

// ============================================================================
// WORLD IMPLEMENTATION
// ============================================================================
// Step 4 deliverable: World now owns all game objects and can produce a
// raw GameStatePacket snapshot without any match logic (score, timer, rules).
// ============================================================================

// ----------------------------------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------------------------------
// SANTI: Setup pitch bounds and call deterministic kickoff initialization.
World::World()
{
	// Define playable area from (0,0) to (window width, window height).
	mPitchBounds = sf::FloatRect(
		sf::Vector2f(0.f, 0.f),
		sf::Vector2f(
			static_cast<float>(Config::WINDOW_WIDTH),
			static_cast<float>(Config::WINDOW_HEIGHT)
		)
	);

	// Place all objects in their initial kickoff positions.
	resetKickoff();
}

// ----------------------------------------------------------------------------
// DETERMINISTIC KICKOFF RESET
// ----------------------------------------------------------------------------
// SANTI: Called by constructor and also used when a goal is scored (to restart).
void World::resetKickoff()
{
	// 1) Initialize all 8 players (team assignment, goalkeeper flag).
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const int teamId = (i < 4) ? Config::HOME_TEAM_SIDE : Config::AWAY_TEAM_SIDE;
		const bool isGoalkeeper = (i == 3 || i == 7);   // IDs 3 (home GK) and 7 (away GK)

		mPlayers[i].init(static_cast<int>(i), teamId, isGoalkeeper);

		// Visual properties (optional but helpful for debugging).
		// mPlayers[i].setRadius(Config::PLAYER_HALF_SIZE);
		// mPlayers[i].setOrigin(sf::Vector2f(mPlayers[i].getRadius(), mPlayers[i].getRadius()));
		// mPlayers[i].setFillColor(teamId == 0 ? sf::Color::White : sf::Color(220, 220, 255));
	}

	// 2) Place goalkeepers at their fixed X positions, centered vertically.
	mPlayers[3].setPosition(sf::Vector2f(Config::GOALKEEPER_X_LEFT, Config::FIELD_CENTER_Y));
	mPlayers[7].setPosition(sf::Vector2f(Config::GOALKEEPER_X_RIGHT, Config::FIELD_CENTER_Y));

	// 3) Place outfield players (3 per team) using formation constants.
	//    Home team (indices 0,1,2) on the left, Away team (4,5,6) on the right.
	for (int j = 0; j < 3; ++j) {
		const float yOffset = Config::FORMATION_CENTER_Y + (j - 1) * Config::FORMATION_PLAYER_SPACING_Y;

		// Home outfield players
		mPlayers[j].setPosition(sf::Vector2f(
			Config::FORMATION_HOME_START_X + j * Config::FORMATION_PLAYER_SPACING_X,
			yOffset
		));

		// Away outfield players
		mPlayers[4 + j].setPosition(sf::Vector2f(
			Config::FORMATION_AWAY_START_X + j * Config::FORMATION_PLAYER_SPACING_X,
			yOffset
		));
	}

	// 4) Reset ball to center, clear any owner, stop movement.
	mBall.setPosition(sf::Vector2f(Config::FIELD_CENTER_X, Config::FIELD_CENTER_Y));
	mBall.clearOwner();
	mBall.setVelocity(sf::Vector2f(0.f, 0.f));
}

// ----------------------------------------------------------------------------
// SNAPSHOT PRODUCER (Step 4 deliverable)
// ----------------------------------------------------------------------------
// SANTI: Fill only the fields that World owns. Does NOT set score, timer, or
//        game state - those belong to Match.
void World::writeRawState(GameStatePacket& out) const
{
	// Pitch boundaries (constant per match).
	out.pitchBounds = mPitchBounds;

	// Ball state.
	out.ballPosition = mBall.getPosition();
	out.ballVelocity = mBall.getVelocity();
	out.ballOwnerPlayerId = static_cast<std::int8_t>(mBall.getOwner());

	// Player states (all 8).
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		const Player& player = mPlayers[i];
		PlayerState& playerState = out.players[i];

		playerState.position = player.getPosition();
		playerState.velocity = player.getVelocity();
		playerState.facingDirection = player.getFacingDirection();
		playerState.teamId = static_cast<std::uint8_t>(player.getTeam());
		playerState.isGoalkeeper = player.IsGoalkeeper();
		playerState.isLunging = player.IsLunging();
	}
}

// ============================================================================
// SUMMARY OF SANTI CHANGES (Step 4)
// ============================================================================
// 1) World::World() constructor:
//      - Sets mPitchBounds using Config::WINDOW_WIDTH/HEIGHT.
//      - Calls resetKickoff() to initialize all objects.
// 2) World::resetKickoff():
//      - Initializes 8 players with correct teams and goalkeeper flags.
//      - Positions goalkeepers at fixed X (GOALKEEPER_X_LEFT/RIGHT) and center Y.
//      - Positions outfield players using formation constants.
//      - Resets ball to center, clears owner, zeroes velocity.
// 3) World::writeRawState(GameStatePacket& out) const:
//      - Fills only World-owned fields: pitchBounds, ball (pos/vel/owner),
//        and all players (position, velocity, facing, team, isGK, isLunging).
//      - Does NOT touch score, timer, currentState, or controlled player IDs.
// ============================================================================
