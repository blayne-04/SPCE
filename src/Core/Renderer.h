#pragma once
#include <SFML/Graphics.hpp>
#include "Common/Packets.h"
#include <optional>
#include <array>

/*Credit: Ryan*/
class Renderer {
public:
    Renderer();
    void render(sf::RenderWindow& window, const GameStatePacket& gameState);
    void renderHUD(sf::RenderWindow& window, int homeScore, int awayScore, float matchTimerSec, int stateID);

private:
    // Background assets
    sf::Texture mBackgroundTex;
    std::optional<sf::Sprite> mBackgroundSprite;

    // SANTI: COWS 29/04/26
    // Chaos event sprite (optional). If the texture fails to load at runtime,
    // Renderer falls back to drawing a simple shape so the game still runs.
    sf::Texture mCowTexture;
    std::optional<sf::Sprite> mCowSprite;

    // SANTI: COWS 30/04/26
    // Idle cow sheet: used only when a cow is paused/not moving.
    sf::Texture mCowIdleTexture;
    std::optional<sf::Sprite> mCowIdleSprite;

    // SANTI 30/04/26
    // Ball icon sheet: used to animate the ball while it is moving.
    sf::Texture mBallTexture;
    std::optional<sf::Sprite> mBallSprite;

    // Player Animation assets
    // [0]=Home/Red/Color1, [1]=Away/Blue/Color6
    // [0]=Down, [1]=Up, [2]=Left, [3]=Right
    sf::Texture mTeamTextures[2][4];
    std::optional<sf::Sprite> mPlayerSprite;

    // Animation constants
    const float ANIM_SPEED = 0.15f;
    const int FRAME_WIDTH = 24;
    const int FRAME_HEIGHT = 24;
};
