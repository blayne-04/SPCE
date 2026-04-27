#pragma once

#include <SFML/Network.hpp>

namespace Config {
	constexpr unsigned int WINDOW_WIDTH = 800;
	constexpr unsigned int WINDOW_HEIGHT = 600;
	inline const sf::Vector2u WINDOW_SIZE(WINDOW_WIDTH, WINDOW_HEIGHT);

	constexpr float FIELD_CENTER_X = WINDOW_WIDTH / 2.f;
	constexpr float FIELD_CENTER_Y = WINDOW_HEIGHT / 2.f;

	constexpr float GOAL_WIDTH = 20.f;
	constexpr float GOAL_HEIGHT = 100.f;
	constexpr float LEFT_GOAL_X = 0.f;
	constexpr float RIGHT_GOAL_X = WINDOW_WIDTH - GOAL_WIDTH;
	constexpr float GOAL_Y_TOP = 250.f;
	constexpr float GOAL_Y_BOTTOM = GOAL_Y_TOP + GOAL_HEIGHT;
	constexpr float GOAL_CENTER_Y = (GOAL_Y_TOP + GOAL_Y_BOTTOM) / 2.f;

	constexpr float PLAYER_SPEED = 200.f;
	constexpr float PLAYER_SIZE = 20.f;
	constexpr float PLAYER_HALF_SIZE = PLAYER_SIZE / 2.f;
	constexpr float PLAYER_MIN_X = PLAYER_HALF_SIZE;
	constexpr float PLAYER_MAX_X = WINDOW_WIDTH - PLAYER_HALF_SIZE;
	constexpr float PLAYER_MIN_Y = PLAYER_HALF_SIZE;
	constexpr float PLAYER_MAX_Y = WINDOW_HEIGHT - PLAYER_HALF_SIZE;
	constexpr float BALL_ATTACH_OFFSET_X = PLAYER_HALF_SIZE;
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
	constexpr float AI_PRESS_SPEED_MULTIPLIER = 1.0f;
	constexpr float AI_SUPPORT_SPEED_MULTIPLIER = 0.9f;
	constexpr float AI_HOLD_SPEED_MULTIPLIER = 0.75f;
	constexpr float AI_TARGET_REACHED_EPSILON = 1.f;
	constexpr float AI_CONTAIN_OFFSET_X = 70.f;
	constexpr float AI_PRESS_IN_OPP_HALF_SPEED_MULTIPLIER = 0.55f;
	constexpr float AI_ATTACK_RUN_SPEED_MULTIPLIER = 1.05f;

	constexpr float PLAYER_MIN_SEPARATION = PLAYER_SIZE * 0.95f;
	constexpr float PLAYER_SEPARATION_PUSH = 125.f;

	constexpr float BALL_RADIUS = 8.f;
	constexpr float BALL_FRICTION = 0.98f;
	constexpr float BALL_STEAL_RADIUS = 26.f;
	constexpr float BALL_STEAL_COOLDOWN_SECONDS = 0.45f;
	constexpr float AI_AUTO_STEAL_ATTEMPTS_PER_SECOND = 2.4f;
	constexpr float HUMAN_MANUAL_STEAL_ATTEMPTS_PER_SECOND = 7.5f;
	constexpr float HUMAN_MANUAL_TACKLE_RADIUS = 44.f;
	constexpr float BALL_POSSESSION_PROTECTION_SECONDS = 0.45f;
	constexpr float STEALER_RETRY_COOLDOWN_SECONDS = 0.35f;

	constexpr float AI_BALL_ACTION_INTERVAL_SECONDS = 0.25f;
	constexpr float AI_ATTACK_INTENT_DURATION_SECONDS = 0.8f;
	constexpr float AI_BALL_CARRY_SPEED_MULTIPLIER = 0.8f;
	constexpr float AI_BALL_CARRY_UNDER_PRESSURE_SPEED_MULTIPLIER = 0.55f;
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
	constexpr float PASS_FORCE = 300.f;
	constexpr float PASS_FORCE_MIN = 260.f;
	constexpr float PASS_FORCE_MAX = 640.f;
	constexpr float PASS_FORCE_PER_PIXEL = 6.5f;
	constexpr float GUARANTEED_PASS_SPEED = 900.f;
	constexpr float GUARANTEED_PASS_MIN_DURATION = 0.10f;
	constexpr float PASS_INTERCEPTION_CORRIDOR_RADIUS = 18.f;
	constexpr float SHOOT_FORCE = 500.f;
	constexpr float SHOOT_FORCE_MIN = 280.f;
	constexpr float SHOOT_FORCE_MAX = 620.f;
	constexpr float SHOOT_CHARGE_MIN_SECONDS = 0.08f;
	constexpr float SHOOT_CHARGE_MAX_SECONDS = 1.0f;
	constexpr float GUARANTEED_SHOT_SPEED_MIN = 850.f;
	constexpr float GUARANTEED_SHOT_SPEED_MAX = 1400.f;
	constexpr float SHOT_INTERCEPTION_CORRIDOR_RADIUS = 20.f;
	constexpr float POST_KICK_PICKUP_DELAY_SECONDS = 0.12f;

	constexpr int OUTFIELD_PLAYERS = 3;
	constexpr float GOALKEEPER_X_LEFT = 60.f;
	constexpr float GOALKEEPER_X_RIGHT = 740.f;
	constexpr float GOALKEEPER_TRACKING_SMOOTHING = 0.1f;

	constexpr float FORMATION_HOME_START_X = 150.f;
	constexpr float FORMATION_AWAY_START_X = 500.f;
	constexpr float FORMATION_PLAYER_SPACING_X = 60.f;
	constexpr float FORMATION_CENTER_Y = 300.f;
	constexpr float FORMATION_PLAYER_SPACING_Y = 40.f;

	constexpr float MATCH_DURATION_SECONDS = 180.f;
	constexpr int WIN_GOAL_LIMIT = 5;
	constexpr float GOAL_REPLAY_DURATION_SECONDS = 1.2f;

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

	// Left hand (movement)
	inline constexpr sf::Keyboard::Key MOVE_UP_KEY = sf::Keyboard::Key::W;
	inline constexpr sf::Keyboard::Key MOVE_DOWN_KEY = sf::Keyboard::Key::S;
	inline constexpr sf::Keyboard::Key MOVE_LEFT_KEY = sf::Keyboard::Key::A;
	inline constexpr sf::Keyboard::Key MOVE_RIGHT_KEY = sf::Keyboard::Key::D;

	// Right hand (actions) - JKL cluster
	inline constexpr sf::Keyboard::Key SHOOT_KEY = sf::Keyboard::Key::K;           // primary shot
	inline constexpr sf::Keyboard::Key PASS_KEY = sf::Keyboard::Key::J;            // short pass
	inline constexpr sf::Keyboard::Key TACKLE_KEY = sf::Keyboard::Key::L;          // standing tackle
	inline constexpr sf::Keyboard::Key SWITCH_PLAYER_KEY = sf::Keyboard::Key::I;   // change controlled player

	// Quit (left hand, easy access)
	inline constexpr sf::Keyboard::Key QUIT_KEY = sf::Keyboard::Key::Q;

	constexpr float PASS_CONTROL_SWITCH_TIMEOUT_SECONDS = 1.25f;
	constexpr float PASS_TARGET_ALIGNMENT_MIN = 0.2f;

	constexpr const char* FONT_PATH = "../assets/fonts/arial.ttf";
