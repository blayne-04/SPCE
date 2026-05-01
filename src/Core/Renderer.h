#pragma once

/**
 * @file Renderer.h
 * @brief Snapshot-only SFML renderer for the soccer game.
 *
 * AI assistance disclosure:
 * A generative AI assistant was used in a limited way to help draft/format documentation
 * comments and to sanity-check the public API for a "snapshot-only" renderer (consume
 * `GameStatePacket`, draw, do not mutate simulation/networking). The team implemented the
 * renderer and validated visuals through play-testing.
 *
 * Example prompt used:
 * "Review this C++/SFML renderer header. Suggest concise Doxygen comments and a clean
 * snapshot-only render API that draws players, ball, cows, HUD, and overlays from
 * GameStatePacket without mutating simulation/networking state."
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
 */
class Renderer {
public:
	Renderer();

	/**
	 * @brief Draw one authoritative GameStatePacket snapshot.
	 *
	 * Renderer does not mutate Match/World; it only consumes packet data.
	 *
	 * @param window Target SFML window.
	 * @param gameState Snapshot to render.
	 * @param showAwayControlledIndicator When false, hide the away-side controlled marker.
	 * @param showGameOverOverlay When false, suppress the full-screen game-over overlay.
	 */
	void render(
		sf::RenderWindow& window,
		const GameStatePacket& gameState,
		bool showAwayControlledIndicator = true,
		bool showGameOverOverlay = true);

	/**
	 * @brief Draw score, clock, and match-state label over the world view.
	 */
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

	// Chaos event sprite (optional). If the texture fails to load at runtime,
	// Renderer falls back to drawing a simple shape so the game still runs.
	sf::Texture mCowTexture;
	std::optional<sf::Sprite> mCowSprite;

	// Idle cow sheet: used only when a cow is paused/not moving.
	sf::Texture mCowIdleTexture;
	std::optional<sf::Sprite> mCowIdleSprite;

	// Ball icon sheet: used to animate the ball while it is moving.
	sf::Texture mBallTexture;
	std::optional<sf::Sprite> mBallSprite;

	// Controlled-player indicator uses the first frame on row 2 of Ball_Icons.png.
	// It is a separate sprite from mBallSprite so drawing the indicator does not
	// overwrite the ball sprite's texture rectangle, origin, scale, or position.
	std::optional<sf::Sprite> mControlledPlayerIndicatorSprite;

	// Player skin animation assets.
	// Each player slot gets its own skin color folder from CharacterSheets/Color2
	// through CharacterSheets/Color10. This keeps skin tone independent from team.
	// [playerId 0-7][0]=Down, [1]=Up, [2]=Left, [3]=Right
	std::array<std::array<sf::Texture, 4>, Config::kNumPlayers> mPlayerSkinTextures{};

	// Team uniform animation assets.
	// Uniforms are team-level: everyone on Home shares one shirt+pants combo,
	// and everyone on Away shares a different shirt+pants combo.
	// [team 0-1][0]=Down, [1]=Up, [2]=Left, [3]=Right
	std::array<std::array<sf::Texture, 4>, 2> mTeamShirtTextures{};
	std::array<std::array<sf::Texture, 4>, 2> mTeamPantTextures{};

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

	// Game-over overlay art. Renderer draws this only when the authoritative
	// snapshot reports currentState == GameOver.
	sf::Texture mGameOverPanelTex;
	std::optional<sf::Sprite> mGameOverPanelSprite;

	sf::Texture mGameOverTextTex;
	std::optional<sf::Sprite> mGameOverTextSprite;

	sf::Texture mTryAgainTex;
	std::optional<sf::Sprite> mTryAgainSprite;

	sf::Texture mQuitTex;
	std::optional<sf::Sprite> mQuitSprite;
};
