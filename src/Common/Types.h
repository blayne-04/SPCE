#pragma once

/* MainMenu = 0, SettingsMenu = 1, PlayingMatch = 2 */
enum class AppState {
    MainMenu,
    SettingsMenu,
    PlayingMatch
};

/* None = 0, Kickoff = 1, Playing = 2, Goal = 3, GameEnd = 4 */
enum class NextMatchState {
    None,
    Kickoff,
    Playing,
    Goal,
    GameEnd
};

/* TEAM1 = 0, TEAM2 = 1 */
enum class TEAMS {
    TEAM1 = 0,
    TEAM2 = 1
};