#pragma once

/**
 * @file Test.h
 * @brief Lightweight unit tests for PA9.
 *
 * This header provides a minimal, dependency-free test runner that is invoked
 * from `main.cpp` in Debug builds.
 */

#include <iostream>
#include <cassert>
#include <cmath>

#include "Core/GameEngine.h"
#include "Simulation/Match.h"
#include "Simulation/World.h"
#include "States/MatchStates.h"
#include "Common/Packets.h"
#include "Common/Constants.h"

namespace Test
{
	/**
	 * @brief Run all unit tests.
	 *
	 * Called by `main.cpp` in Debug builds. Keep this non-interactive (no windows)
	 * so it works on CI and in headless environments.
	 */
	void runTests();

	void testMatchResetDefaults();
	void testKickoffPassTransitionsToPlaying();
	void testDefensiveControlSwitch();
	void testNoPlayersInsideGoalMouth();
	void testPacketSerializationRoundTrip();

	bool approxEqual(float a, float b, float epsilon = 0.001f);
	void assertVectorNear(const sf::Vector2f& a, const sf::Vector2f& b, float epsilon = 0.001f);
	void assertFloatRectNear(const sf::FloatRect& a, const sf::FloatRect& b, float epsilon = 0.001f);
}

inline bool Test::approxEqual(float a, float b, float epsilon) {
	return std::abs(a - b) <= epsilon;
}

inline void Test::assertVectorNear(const sf::Vector2f& a, const sf::Vector2f& b, float epsilon) {
	assert(approxEqual(a.x, b.x, epsilon));
	assert(approxEqual(a.y, b.y, epsilon));
}

inline void Test::assertFloatRectNear(const sf::FloatRect& a, const sf::FloatRect& b, float epsilon) {
	assert(approxEqual(a.position.x, b.position.x, epsilon));
	assert(approxEqual(a.position.y, b.position.y, epsilon));
	assert(approxEqual(a.size.x, b.size.x, epsilon));
	assert(approxEqual(a.size.y, b.size.y, epsilon));
}


inline void Test::testMatchResetDefaults() {
	std::cout << "\n=== Test: Match::reset defaults ===" << std::endl;

	Match match;
	match.incrementScore(TEAMS::TEAM1);
	match.incrementScore(TEAMS::TEAM2);
	match.addTimeSec(12.5f);
	match.setControlledPlayerIds(2, 6);
	match.setKickoffTeamSide(Config::AWAY_TEAM_SIDE);

	match.getWorld().ball().setOwner(5);
	match.getWorld().ball().setPosition({ 100.f, 120.f });

	match.reset();

	GameStatePacket state{};
	match.getGameState(state);

	assert(state.scoreHome == 0);
	assert(state.scoreAway == 0);
	assert(approxEqual(state.matchTimerSec, 0.f));
	assert(state.controlledHomePlayerId == 0);
	assert(state.controlledAwayPlayerId == 4);
	assert(state.possessingTeamId == -1);
	assert(state.currentState == 0);

	std::cout << "  -> PASS\n" << std::endl;
}

inline void Test::testKickoffPassTransitionsToPlaying() {
	std::cout << "\n=== Test: Kickoff pass transitions to Playing ===" << std::endl;

	Match match;
	const int kickoffOwnerId = (match.getKickoffTeamSide() == Config::AWAY_TEAM_SIDE) ? 4 : 0;

	FrameInput frame{};
	frame.inputs[static_cast<std::size_t>(kickoffOwnerId)].passDown = true;

	match.update(frame, 0.016f);

	GameStatePacket state{};
	bool transitioned = false;

	for (int i = 0; i < 240; ++i) {
		FrameInput nextFrame{};
		match.update(nextFrame, 0.016f);
		match.getGameState(state);
		if (state.currentState == 1) {
			transitioned = true;
			break;
		}
	}

	assert(transitioned);

	std::cout << "  -> PASS\n" << std::endl;
}

inline void Test::testDefensiveControlSwitch() {
	std::cout << "\n=== Test: Defensive control switch ===" << std::endl;

	Match match;
	match.TransitionTo(std::make_unique<PlayingState>());

	World& world = match.getWorld();
	world.ball().setOwner(4);
	world.ball().setPosition({ 400.f, 300.f });

	world.players()[0].setPosition({ 300.f, 300.f });
	world.players()[1].setPosition({ 380.f, 300.f });
	world.players()[2].setPosition({ 350.f, 300.f });

	FrameInput frame{};
	frame.inputs[0].switchDown = true;

	match.update(frame, 0.016f);

	const std::uint8_t controlledHome = match.getControlledHomePlayerId();
	assert(controlledHome == 2);

	std::cout << "  -> PASS\n" << std::endl;
}

inline void Test::testNoPlayersInsideGoalMouth() {
	std::cout << "\n=== Test: enforceNoPlayersInsideGoalMouth ===" << std::endl;

	World world;

	const float leftGoalEdge = Config::LEFT_GOAL_X + Config::GOAL_WIDTH + Config::PLAYER_HALF_SIZE;
	const float rightGoalEdge = Config::RIGHT_GOAL_X - Config::PLAYER_HALF_SIZE;

	world.players()[0].setPosition({ Config::LEFT_GOAL_X + 2.f, Config::GOAL_CENTER_Y });
	world.players()[4].setPosition({ Config::RIGHT_GOAL_X + 2.f, Config::GOAL_CENTER_Y });

	world.enforceNoPlayersInsideGoalMouth();

	const sf::Vector2f leftPos = world.players()[0].getPosition();
	const sf::Vector2f rightPos = world.players()[4].getPosition();

	assert(leftPos.x >= leftGoalEdge - 0.01f);
	assert(rightPos.x <= rightGoalEdge + 0.01f);

	std::cout << "  -> PASS\n" << std::endl;
}

inline void Test::testPacketSerializationRoundTrip() {
	std::cout << "\n=== Test: Packet serialization round-trip ===" << std::endl;

	InputPacket input{};
	input.inputSequence = 42;
	input.playerId = 3;
	input.moveDirection = { -0.75f, 0.5f };
	input.shootDown = true;
	input.passDown = false;
	input.tackleDown = true;
	input.switchDown = true;
	input.lungeDown = false;

	sf::Packet inputPacket;
	inputPacket << input;

	InputPacket inputOut{};
	inputPacket >> inputOut;

	assert(inputOut.inputSequence == input.inputSequence);
	assert(inputOut.playerId == input.playerId);
	assertVectorNear(inputOut.moveDirection, input.moveDirection);
	assert(inputOut.shootDown == input.shootDown);
	assert(inputOut.passDown == input.passDown);
	assert(inputOut.tackleDown == input.tackleDown);
	assert(inputOut.switchDown == input.switchDown);
	assert(inputOut.lungeDown == input.lungeDown);

	GameStatePacket state{};
	state.frameNumber = 99;
	state.ballPosition = { 123.4f, 567.8f };
	state.ballVelocity = { -10.5f, 12.25f };
	state.ballOwnerPlayerId = 2;
	state.possessingTeamId = 1;
	state.controlledHomePlayerId = 1;
	state.controlledAwayPlayerId = 5;
	state.scoreHome = 3;
	state.scoreAway = 2;
	state.matchTimerSec = 45.5f;
	state.pitchBounds = sf::FloatRect({ 0.f, 0.f }, { 800.f, 600.f });
	state.currentState = 1;

	for (std::size_t i = 0; i < state.players.size(); ++i) {
		state.players[i].position = { 10.f + static_cast<float>(i), 20.f + static_cast<float>(i) };
		state.players[i].velocity = { 1.f, -2.f };
		state.players[i].facingDirection = { 0.f, 1.f };
		state.players[i].teamId = static_cast<std::uint8_t>(i < 4 ? 0 : 1);
		state.players[i].isGoalkeeper = (i == 3 || i == 7);
		state.players[i].isLunging = (i % 2 == 0);
	}

	for (std::size_t i = 0; i < state.cows.size(); ++i) {
		state.cows[i].active = (i % 2 == 0);
		state.cows[i].position = { 200.f + static_cast<float>(i), 100.f + static_cast<float>(i) };
		state.cows[i].velocity = { -1.f, 2.f };
	}

	sf::Packet statePacket;
	statePacket << state;

	GameStatePacket stateOut{};
	statePacket >> stateOut;

	assert(stateOut.frameNumber == state.frameNumber);
	assertVectorNear(stateOut.ballPosition, state.ballPosition);
	assertVectorNear(stateOut.ballVelocity, state.ballVelocity);
	assert(stateOut.ballOwnerPlayerId == state.ballOwnerPlayerId);
	assert(stateOut.possessingTeamId == state.possessingTeamId);
	assert(stateOut.controlledHomePlayerId == state.controlledHomePlayerId);
	assert(stateOut.controlledAwayPlayerId == state.controlledAwayPlayerId);
	assert(stateOut.scoreHome == state.scoreHome);
	assert(stateOut.scoreAway == state.scoreAway);
	assert(approxEqual(stateOut.matchTimerSec, state.matchTimerSec));
	assertFloatRectNear(stateOut.pitchBounds, state.pitchBounds);
	assert(stateOut.currentState == state.currentState);

	for (std::size_t i = 0; i < state.players.size(); ++i) {
		assertVectorNear(stateOut.players[i].position, state.players[i].position);
		assertVectorNear(stateOut.players[i].velocity, state.players[i].velocity);
		assertVectorNear(stateOut.players[i].facingDirection, state.players[i].facingDirection);
		assert(stateOut.players[i].teamId == state.players[i].teamId);
		assert(stateOut.players[i].isGoalkeeper == state.players[i].isGoalkeeper);
		assert(stateOut.players[i].isLunging == state.players[i].isLunging);
	}

	for (std::size_t i = 0; i < state.cows.size(); ++i) {
		assert(stateOut.cows[i].active == state.cows[i].active);
		assertVectorNear(stateOut.cows[i].position, state.cows[i].position);
		assertVectorNear(stateOut.cows[i].velocity, state.cows[i].velocity);
	}

	std::cout << "  -> PASS\n" << std::endl;
}

inline void Test::runTests() {
	// This is the entry point called by main.cpp in Debug builds.
	// Keep it pure unit tests (no SFML windows) so CI/local runs do not hang.
	std::cout << "\n===================================\n";
	std::cout << "Running unit tests (Debug only)...\n";
	std::cout << "===================================\n" << std::endl;

	// 1) Match reset and defaults.
	testMatchResetDefaults();

	// 2) Kickoff rules: kickoff pass should transition to Playing.
	testKickoffPassTransitionsToPlaying();

	// 3) Control switching logic: defensive switching should pick nearest defender.
	testDefensiveControlSwitch();

	// 4) Rule enforcement: no players inside the goal mouth area.
	testNoPlayersInsideGoalMouth();

	// 5) Networking contract safety: packet serialization round-trip.
	testPacketSerializationRoundTrip();

	std::cout << "===================================\n";
	std::cout << "All unit tests passed.\n";
	std::cout << "===================================\n" << std::endl;
}

