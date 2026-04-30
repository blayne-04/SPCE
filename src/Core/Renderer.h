#pragma once

/**
 * @file Renderer.h
 * @brief Snapshot-only SFML renderer for the soccer game.
 *
 * AI disclosure:
 * The layered sprite renderer, randomized skin/uniform selection, cow animation,
 * ball animation, and fallback rendering were written and documented with help
 * from OpenAI Codex because this rendering pipeline is more complex than a
 * typical CPTS 122 graphics requirement.
 *
 * Prompt used:
 * "Help me implement an SFML renderer that consumes only GameStatePacket. Draw
 * animated players from layered Penzilla body/shirt/pants sheets, randomize
 * realistic skin colors and team uniforms, animate cows and ball sprites, and
 * keep all gameplay/networking out of Renderer."
 */

#include <SFML/Graphics.hpp>
#include "Common/Constants.h"
#include "Common/Packets.h"
#include <optional>
#include <array>

/**
 * @class Renderer
 * @brief Draws the current authoritative game snapshot.
 *
 * Renderer intentionally does not own or modify World, Match, NetworkManager,
 * or InputHandler. It only displays the snapshot it receives.
 *
 * Credit: Ryan contributed the initial player sprite rendering approach.
 */
class Renderer {
public:
    Renderer();

    // SANTI 30/04/26
    // Draws one authoritative GameStatePacket snapshot.
    // Renderer does not mutate Match/World; it only consumes packet data.
    void render(sf::RenderWindow& window, const GameStatePacket& gameState);

    // SANTI 30/04/26
    // Draws score, clock, and current match-state label over the world view.
    void renderHUD(
        sf::RenderWindow& window,
        int homeScore,
        int awayScore,
        float matchTimerSec,
        int stateId);

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

    // SANTI 30/04/26
    // Player skin animation assets.
    // Each player slot gets its own skin color folder from CharacterSheets/Color2
    // through CharacterSheets/Color10. This keeps skin tone independent from team.
    // [playerId 0-7][0]=Down, [1]=Up, [2]=Left, [3]=Right
    std::array<std::array<sf::Texture, 4>, Config::kNumPlayers> mPlayerSkinTextures{};

    // SANTI 30/04/26
    // Team uniform animation assets.
    // Uniforms are team-level: everyone on Home shares one shirt+pants combo,
    // and everyone on Away shares a different shirt+pants combo.
    // [team 0-1][0]=Down, [1]=Up, [2]=Left, [3]=Right
    std::array<std::array<sf::Texture, 4>, 2> mTeamShirtTextures{};
    std::array<std::array<sf::Texture, 4>, 2> mTeamPantTextures{};

    // SANTI 30/04/26
    // Goalkeeper uniform assets.
    // Goalkeepers always wear all black, independent of the randomized outfield
    // uniforms. This keeps keeper identity clear and prevents accidental kit clash.
    // [0]=Down, [1]=Up, [2]=Left, [3]=Right
    std::array<sf::Texture, 4> mGoalkeeperShirtTextures{};
    std::array<sf::Texture, 4> mGoalkeeperPantTextures{};

    std::optional<sf::Sprite> mPlayerSprite;

    // Animation constants
    const float ANIM_SPEED = 0.15f;
    const int FRAME_WIDTH = 24;
    const int FRAME_HEIGHT = 24;
};
