#pragma once
#include "../Common/Packets.h"

/* Forward declaration to avoid circular dependency */
class Match;

class MatchState {
public:
    MatchState() = default;

    virtual ~MatchState() = default;

    /* Contextual update function, called by Match to delegate frame updates to the current state of the program */
    virtual void update(Match& match, const FrameInput& frameData) = 0;

    /* Contextual function to be called when entering a new state */
    virtual void onEnter(Match& match) = 0;

	/* Contextual function to be called when exiting a state */
    virtual void onExit(Match& match) = 0;
};

/* ========================================= */
/*              GAME OVER                    */
/* ========================================= */

class GameOverState : public MatchState
{
public:
    GameOverState();
    void update(Match& match, const FrameInput& frameData) override;
    void onEnter(Match& match) override;
    void onExit(Match& match) override;
};

/* ========================================= */
/*                KICKOFF                    */
/* ========================================= */
class KickoffState : public MatchState
{
public:
    KickoffState();
    void update(Match& match, const FrameInput& frameData) override;
    void onEnter(Match& match) override;
    void onExit(Match& match) override;
};

/* ========================================= */
/*                PLAYING                    */
/* ========================================= */
class PlayingState: public MatchState
{
    public:
    PlayingState();
    void update(Match& match, const FrameInput& frameData) override;
    void onEnter(Match& match) override;
    void onExit(Match& match) override;
};