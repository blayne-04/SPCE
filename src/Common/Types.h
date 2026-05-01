#pragma once

/**
 * @file Types.h
 * @brief Small shared enums for application and match state names.
 */

 /** @brief High-level application screen categories. */
enum class AppState {
	MainMenu,
	SettingsMenu,
	PlayingMatch
};

/** @brief Possible next match-state requests. */
enum class NextMatchState {
	None,
	Kickoff,
	Playing,
	Goal,
	GameEnd
};

/** @brief Score/team identifiers used by Match. */
enum class TEAMS {
	TEAM1 = 0,
	TEAM2 = 1
};
