#pragma once

/**
 * @file Constants.h
 * @brief Central tuning table for rendering, networking, physics, AI, cows, and controls.
 *
 * AI disclosure:
 * Some balancing constants and explanatory comments in this file were refined
 * with help from OpenAI Codex while debugging gameplay feel and networking.
 *
 * Prompt used:
 * "Review this SFML soccer game and suggest clear constants for AI difficulty,
 * goalkeeper behavior, cow chaos events, networking, and packet-safe gameplay."
 */

 // SANTI: Fixed includes.
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

// SANTI: std::size_t lives in <cstddef> (there is no <size_t> header).
#include <cstddef>

// ============================================================================
// CONFIGURATION NAMESPACE
// ============================================================================
// Central location for all game constants: window, field, physics, AI, input,
// networking, etc. Change values here to tweak gameplay.
// ============================================================================

namespace Config {

	// ----------------------------------------------------------------------------
	// WINDOW & FIELD DIMENSIONS
	// ----------------------------------------------------------------------------

	constexpr unsigned int WINDOW_WIDTH = 800;
	constexpr unsigned int WINDOW_HEIGHT = 600;
	inline const sf::Vector2u WINDOW_SIZE(WINDOW_WIDTH, WINDOW_HEIGHT);

	constexpr float FIELD_CENTER_X = WINDOW_WIDTH / 2.f;
	constexpr float FIELD_CENTER_Y = WINDOW_HEIGHT / 2.f;

	// ----------------------------------------------------------------------------
	// KICKOFF (SANTI 28/04/2026)
	// ----------------------------------------------------------------------------
	// Real-football kickoff constraint: the defending team must stay outside the
	// center circle until the kickoff pass is completed.
	//
	// NOTE: This is a gameplay constant (not networking). It is applied only by
	// the host simulation during KickoffState.
	// SANTI 28/04/2026: Tuned smaller so kickoff looks less "locked down" on
	// an 800x600 pitch while still enforcing the rule.
	constexpr float KICKOFF_CIRCLE_RADIUS = 65.f;

	// ----------------------------------------------------------------------------
	// GOAL GEOMETRY
	// ----------------------------------------------------------------------------

	constexpr float GOAL_WIDTH = 20.f;
	constexpr float GOAL_HEIGHT = 100.f;
	constexpr float LEFT_GOAL_X = 0.f;
	constexpr float RIGHT_GOAL_X = WINDOW_WIDTH - GOAL_WIDTH;
	constexpr float GOAL_Y_TOP = 250.f;
	constexpr float GOAL_Y_BOTTOM = GOAL_Y_TOP + GOAL_HEIGHT;
	constexpr float GOAL_CENTER_Y = (GOAL_Y_TOP + GOAL_Y_BOTTOM) / 2.f;

	// ----------------------------------------------------------------------------
	// PLAYER PHYSICS & MOVEMENT
	// ----------------------------------------------------------------------------

	constexpr float PLAYER_SPEED = 200.f;
	constexpr float PLAYER_SIZE = 20.f;
	constexpr float PLAYER_HALF_SIZE = PLAYER_SIZE / 2.f;
	constexpr float PLAYER_MIN_X = PLAYER_HALF_SIZE;
	constexpr float PLAYER_MAX_X = WINDOW_WIDTH - PLAYER_HALF_SIZE;
	constexpr float PLAYER_MIN_Y = PLAYER_HALF_SIZE;
	constexpr float PLAYER_MAX_Y = WINDOW_HEIGHT - PLAYER_HALF_SIZE;
	constexpr float BALL_ATTACH_OFFSET_X = PLAYER_HALF_SIZE;
	constexpr float PLAYER_MIN_SEPARATION = PLAYER_SIZE * 0.95f;
	constexpr float PLAYER_SEPARATION_PUSH = 125.f;

	// ----------------------------------------------------------------------------
	// AI BEHAVIOR (General)
	// ----------------------------------------------------------------------------

	// SANTI 28/04/2026: Difficulty knobs.
	// The AI can feel "too perfect" in an MVP because it has:
	// - zero reaction delay
	// - perfect input (no stick drift / no mis-presses)
	// - deterministic tackle success once in range
	//
	// This multiplier is an easy first step to make singleplayer feel fairer
	// without rewriting the decision logic.
	// SANTI 28/04/2026: Further nerf. AI was still feeling too oppressive in
	// singleplayer playtests because it never makes input mistakes.
	constexpr float AI_DIFFICULTY_SPEED_SCALE = 0.75f;

	// SANTI 28/04/2026: AI should not spam tackles from too far away.
	// Humans can still do that (skill expression), but AI gets a smaller trigger radius.
	// SANTI 28/04/2026: Further nerf. Make AI commit to tackles only when very close.
	// SANTI 29/04/26: Slightly increased so AI tackles actually happen.
	// 15px was so tight that, with separation pushes + ball attach offset, the AI
	// would rarely reach the trigger threshold and "never tackle" looked like a bug.
	constexpr float AI_TACKLE_TRIGGER_RADIUS = 22.f;

	constexpr float AI_SPEED = 150.f;
	constexpr float AI_FORMATION_BALL_INFLUENCE = 0.25f;
	constexpr float AI_ATTACKING_SHAPE_BALL_INFLUENCE = 0.15f;
	constexpr float AI_SUPPORT_OFFSET_X = 70.f;
	constexpr float AI_SUPPORT_SHORT_OFFSET_X = 55.f;
	constexpr float AI_SUPPORT_SHORT_OFFSET_Y = 80.f;
	constexpr float AI_SUPPORT_RUN_AHEAD_X = 180.f;
	constexpr float AI_SUPPORT_RUN_AHEAD_PROGRESS_BONUS_X = 80.f;
	constexpr float AI_SUPPORT_RUN_WIDE_Y = 125.f;
	constexpr float AI_RUN_INTENT_LOCK_MIN_SECONDS = 0.7f;
	constexpr float AI_RUN_INTENT_LOCK_MAX_SECONDS = 1.0f;
	constexpr float AI_2V1_TRIGGER_DISTANCE = 70.f;
	constexpr float AI_2V1_RUN_BONUS_X = 110.f;
	constexpr float AI_LANE_WIDE_Y = 150.f;
	constexpr float AI_LANE_HALFSPACE_Y = 80.f;
	constexpr float AI_LANE_DIVERSITY_MIN_DISTANCE = 55.f;

	// AI Speed Multipliers
	constexpr float AI_PRESS_SPEED_MULTIPLIER = 1.0f;
	constexpr float AI_SUPPORT_SPEED_MULTIPLIER = 0.9f;
	constexpr float AI_HOLD_SPEED_MULTIPLIER = 0.75f;
	constexpr float AI_PRESS_IN_OPP_HALF_SPEED_MULTIPLIER = 0.55f;
	constexpr float AI_ATTACK_RUN_SPEED_MULTIPLIER = 1.05f;
	constexpr float AI_BALL_CARRY_SPEED_MULTIPLIER = 0.8f;
	constexpr float AI_BALL_CARRY_UNDER_PRESSURE_SPEED_MULTIPLIER = 0.55f;

	// AI Positioning & Decision
	constexpr float AI_TARGET_REACHED_EPSILON = 1.f;
	constexpr float AI_CONTAIN_OFFSET_X = 70.f;
	constexpr float AI_BALL_CARRY_CENTERING = 0.55f;
	constexpr float AI_BALL_CARRY_EVASION_WEIGHT = 0.75f;
	constexpr float AI_BALL_CARRY_MIN_FORWARD_DOT = 0.45f;
	constexpr float AI_SHOOT_DISTANCE_X = 230.f;
	constexpr float AI_PASS_PRESSURE_DISTANCE = 45.f;
	constexpr float AI_PASS_HIGH_PRESSURE_DISTANCE = 24.f;
	constexpr float AI_PASS_TARGET_SPACE_BONUS_DISTANCE = 85.f;
	constexpr float AI_MIN_PASS_ALIGNMENT = 0.25f;
	constexpr float AI_SHOT_LANE_BLOCK_RADIUS = 26.f;
	constexpr float AI_SHOT_CLEAR_MIN_SCORE = 0.55f;
	constexpr float AI_SHOT_CLEAR_MIN_SCORE_FINAL_THIRD = 0.30f;
	constexpr float AI_FINAL_THIRD_FORCE_ACTION_SECONDS = 0.6f;
	constexpr float AI_SHOT_POST_TARGET_MARGIN_Y = 18.f;
	constexpr float AI_ATTACKING_PROGRESS_FINAL_THIRD = 0.67f;
	constexpr float AI_ATTACK_INTENT_DURATION_SECONDS = 0.8f;
	// SANTI 28/04/2026: Slow down AI pass/shot cadence so it does not feel
	// "frame-perfect" compared to a human.
	constexpr float AI_BALL_ACTION_INTERVAL_SECONDS = 0.33f;

	// ----------------------------------------------------------------------------
	// BALL PHYSICS & STEALING
	// ----------------------------------------------------------------------------

	constexpr float BALL_RADIUS = 8.f;
	constexpr float BALL_FRICTION = 0.98f;
	constexpr float BALL_STEAL_RADIUS = 26.f;
	constexpr float BALL_STEAL_COOLDOWN_SECONDS = 0.45f;
	constexpr float AI_AUTO_STEAL_ATTEMPTS_PER_SECOND = 2.4f;
	constexpr float HUMAN_MANUAL_STEAL_ATTEMPTS_PER_SECOND = 7.5f;
	constexpr float HUMAN_MANUAL_TACKLE_RADIUS = 44.f;
	constexpr float BALL_POSSESSION_PROTECTION_SECONDS = 0.45f;
	constexpr float STEALER_RETRY_COOLDOWN_SECONDS = 0.35f;

	// ----------------------------------------------------------------------------
	// CHAOS EVENT: COW INVASION (SANTI: COWS 29/04/26)
	// ----------------------------------------------------------------------------
	// These constants control the "Copa Peru cows" chaos event.
	// Design constraints for networking safety:
	// - Max cow count is FIXED (no vectors in the packet).
	// - Host simulates cows, clients render from GameStatePacket only.
	inline constexpr std::size_t kMaxCows = 6;

	// Visual / collision radius (world units, same coordinate space as players/ball).
	constexpr float COW_RADIUS = 16.f;

	// Cow movement behavior (grazing is intentionally slow to feel like an obstacle).
	constexpr float COW_SPEED_ENTERING = 95.f;
	constexpr float COW_SPEED_GRAZING = 55.f;

	// Spawn scheduling in PlayingState only.
	// SANTI: COWS 30/04/26
	// Spawns happen over time until kMaxCows is reached. Cows do NOT despawn on goals.
	constexpr float COW_SPAWN_MIN_DELAY_SECONDS = 20.f;
	constexpr float COW_SPAWN_MAX_DELAY_SECONDS = 38.f;

	// Wander behavior: cows alternate between "move" and "pause".
	// SANTI: COWS 30/04/26
	// This produces the "scramble, stand still, scramble, stand still" feel.
	constexpr float COW_WANDER_MOVE_MIN_SECONDS = 0.85f;
	constexpr float COW_WANDER_MOVE_MAX_SECONDS = 2.10f;
	constexpr float COW_WANDER_PAUSE_MIN_SECONDS = 0.65f;
	constexpr float COW_WANDER_PAUSE_MAX_SECONDS = 1.75f;

	// Avoid edges so cows distribute across the field instead of clumping on borders.
	// This is in addition to COW_RADIUS.
	constexpr float COW_EDGE_CLEARANCE = 28.f;

	// Extra margin around the goal mouth to ensure cows do not look like they are
	// "inside the goal" visually, even if sprites/lines are thicker than GOAL_WIDTH.
	constexpr float COW_GOAL_MOUTH_CLEARANCE = 24.f;
	constexpr float COW_TARGET_REACHED_EPSILON = 10.f;

	// Ball collision response when a cow blocks the ball.
	constexpr float COW_BALL_BOUNCE_DAMPING = 0.68f;
	constexpr float COW_BALL_MIN_BOUNCE_SPEED = 140.f;

	// Asset path (runtime-only; safe if missing, renderer will fall back to shapes).
	constexpr const char* COW_TEXTURE_PATH = "assets/textures/cow.png";

	// ----------------------------------------------------------------------------
	// PASSING & SHOOTING
	// ----------------------------------------------------------------------------

	constexpr float PASS_FORCE = 300.f;
	constexpr float PASS_FORCE_MIN = 260.f;
	constexpr float PASS_FORCE_MAX = 640.f;
	constexpr float PASS_FORCE_PER_PIXEL = 6.5f;
	constexpr float GUARANTEED_PASS_SPEED = 900.f;
	constexpr float GUARANTEED_PASS_MIN_DURATION = 0.10f;
	constexpr float PASS_INTERCEPTION_CORRIDOR_RADIUS = 18.f;
	constexpr float PASS_CONTROL_SWITCH_TIMEOUT_SECONDS = 1.25f;
	constexpr float PASS_TARGET_ALIGNMENT_MIN = 0.2f;

	constexpr float SHOOT_FORCE = 500.f;
	constexpr float SHOOT_FORCE_MIN = 280.f;
	constexpr float SHOOT_FORCE_MAX = 620.f;
	constexpr float SHOOT_CHARGE_MIN_SECONDS = 0.08f;
	constexpr float SHOOT_CHARGE_MAX_SECONDS = 1.0f;
	constexpr float GUARANTEED_SHOT_SPEED_MIN = 850.f;
	constexpr float GUARANTEED_SHOT_SPEED_MAX = 1400.f;
	constexpr float SHOT_INTERCEPTION_CORRIDOR_RADIUS = 20.f;
	constexpr float POST_KICK_PICKUP_DELAY_SECONDS = 0.12f;

	// ----------------------------------------------------------------------------
	// SHOOTING ACCURACY / GOALKEEPER SAVES (SANTI 28/04/2026)
	// ----------------------------------------------------------------------------
	// You asked for "probability to score" to correlate with shot distance so
	// players are incentivized to get closer before shooting.
	//
	// Implementation: if a shot is not intercepted along the corridor, the
	// defending goalkeeper gets a distance-based chance to "save" the shot.
	// Saved shots end at the goalkeeper position (so they do NOT count as goals).
	//
	// Distances are measured in X (toward the goal) because your game is mostly
	// left-to-right attack.
	constexpr float SHOT_SAVE_DISTANCE_CLOSE_X = 140.f;
	constexpr float SHOT_SAVE_DISTANCE_FAR_X = 420.f;

	// Save chance at close vs far distances.
	constexpr float SHOT_SAVE_CHANCE_CLOSE = 0.15f;
	constexpr float SHOT_SAVE_CHANCE_FAR = 0.75f;

	// ----------------------------------------------------------------------------
	// TEAM & FORMATION
	// ----------------------------------------------------------------------------

	constexpr int OUTFIELD_PLAYERS = 3;
	constexpr float GOALKEEPER_X_LEFT = 60.f;
	constexpr float GOALKEEPER_X_RIGHT = 740.f;
	constexpr float GOALKEEPER_TRACKING_SMOOTHING = 0.1f;

	// ----------------------------------------------------------------------------
	// GOAL AREA / SIX-YARD BOX (SANTI 28/04/2026)
	// ----------------------------------------------------------------------------
	// Used to prevent attackers from crowding the goalkeeper while the goalkeeper
	// is holding the ball (lets the keeper distribute like real football).
	constexpr float SIX_YARD_BOX_DEPTH = 125.f;
	constexpr float SIX_YARD_BOX_MARGIN_Y = 25.f;

	// SANTI 28/04/2026: If a goalkeeper holds the ball too long, force a distribution
	// pass so the match does not stall (small-sided "6-second rule" style).
	constexpr float GOALKEEPER_HOLD_AUTO_DISTRIBUTE_SECONDS = 4.0f;

	constexpr float FORMATION_HOME_START_X = 150.f;
	constexpr float FORMATION_AWAY_START_X = 500.f;
	constexpr float FORMATION_PLAYER_SPACING_X = 60.f;
	constexpr float FORMATION_CENTER_Y = 300.f;
	constexpr float FORMATION_PLAYER_SPACING_Y = 40.f;

	// ----------------------------------------------------------------------------
	// MATCH RULES & UI
	// ----------------------------------------------------------------------------

	constexpr float MATCH_DURATION_SECONDS = 180.f;
	constexpr int WIN_GOAL_LIMIT = 5;
	constexpr float GOAL_REPLAY_DURATION_SECONDS = 1.2f;
	// SANTI 01/05/2026
	// Delay before the full Game Over menu appears. The match can enter
	// GameOver immediately, but this gives the player one second to see the
	// final field state before the large overlay/buttons cover the screen.
	constexpr float GAME_OVER_MENU_DELAY_SECONDS = 1.0f;

	constexpr unsigned int SCORE_TEXT_SIZE = 24;
	constexpr float SCORE_TEXT_X = 10.f;
	constexpr float SCORE_TEXT_Y = 10.f;
	constexpr unsigned int STATE_TEXT_SIZE = 30;
	constexpr float STATE_TEXT_X = 250.f;
	constexpr float STATE_TEXT_Y = 20.f;

	constexpr int HOME_TEAM_SIDE = 0;
	constexpr int AWAY_TEAM_SIDE = 1;
	constexpr int HUMAN_PLAYER_INDEX = 0;
	constexpr float DEFAULT_KICK_DIRECTION_X = 1.f;
	constexpr float VECTOR_NORMALIZATION_EPSILON = 0.01f;

	// ----------------------------------------------------------------------------
	// INPUT KEY BINDINGS (QWERTY layout)
	// ----------------------------------------------------------------------------

	// Left hand (movement)
	inline constexpr sf::Keyboard::Key MOVE_UP_KEY = sf::Keyboard::Key::W;
	inline constexpr sf::Keyboard::Key MOVE_DOWN_KEY = sf::Keyboard::Key::S;
	inline constexpr sf::Keyboard::Key MOVE_LEFT_KEY = sf::Keyboard::Key::A;
	inline constexpr sf::Keyboard::Key MOVE_RIGHT_KEY = sf::Keyboard::Key::D;

	// Right hand (actions) - JKL cluster
	inline constexpr sf::Keyboard::Key SHOOT_KEY = sf::Keyboard::Key::K;           // primary shot
	inline constexpr sf::Keyboard::Key PASS_KEY = sf::Keyboard::Key::J;            // short pass
	inline constexpr sf::Keyboard::Key TACKLE_KEY = sf::Keyboard::Key::L;          // standing tackle
	// SANTI 29/04/26: Alternate tackle key for old-project muscle memory.
	// Earlier notes used F for "steal/tackle". If you press the wrong key, it
	// looks like tackles are broken, so we accept both.
	inline constexpr sf::Keyboard::Key TACKLE_KEY_ALT = sf::Keyboard::Key::F;      // standing tackle (alt)
	inline constexpr sf::Keyboard::Key SWITCH_PLAYER_KEY = sf::Keyboard::Key::I;   // change controlled player

	// Quit (left hand, easy access)
	inline constexpr sf::Keyboard::Key QUIT_KEY = sf::Keyboard::Key::Q;

	// ----------------------------------------------------------------------------
	// ASSETS
	// ----------------------------------------------------------------------------

	constexpr const char* FONT_PATH = "../assets/fonts/arial.ttf";

	// ----------------------------------------------------------------------------
	// NETWORKING (SANTI: just added HOST_PORT)
	// ----------------------------------------------------------------------------

	// One place to change the port for host + client.
	constexpr unsigned short HOST_PORT = 54000;

	// SANTI 29/04/26: Default host address for client mode.
	// Change this for LAN tests (example: "192.168.1.50").
	// We keep it as a string so Constants.h does not need SFML Network includes.
	constexpr const char* DEFAULT_HOST_ADDRESS = "127.0.0.1";

	// SANTI: changed from 10 to 8 players (4 per team)
	inline constexpr std::size_t kNumPlayers = 8;

} // namespace Config

