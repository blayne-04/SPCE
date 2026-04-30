#include "Renderer.h"
#include "Common/Constants.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {
    const sf::Font* getHudFont() {
        static sf::Font font;
        static bool loaded = font.openFromFile("assets/fonts/arial.ttf");
        return loaded ? &font : nullptr;
    }

    std::string stateNameFromId(int stateId) {
        switch (stateId) {
        case 0: return "Kickoff";
        case 1: return "Playing";
        case 2: return "Goal";
        case 3: return "GameOver";
        default: return "Unknown";
        }
    }

    std::string formatClock(float seconds) {
        int total = std::max(0, static_cast<int>(seconds));
        std::ostringstream oss;
        oss << (total / 60) << ":" << std::setfill('0') << std::setw(2) << (total % 60);
        return oss.str();
    }
}

Renderer::Renderer() {

    //load background
    mSheetTex.loadFromFile("assets/field.png");
    mSheetTex.setRepeated(true);

    mBackgroundSprite.emplace(mSheetTex);
    mBackgroundSprite->setTextureRect(
        sf::IntRect({ 0, 0 }, { Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT })
    );
        

    // 2. Load Teammate's Animated Sheets
    std::string paths[2] = { "Color1/", "Color6/" };
    std::string files[4] = { "Down.png", "Up.png", "Left.png", "Right.png" };
    std::string base = "assets/textures/Penzilla Protagonist/CharacterSheets/";

    for (int team = 0; team < 2; ++team) {
        for (int dir = 0; dir < 4; ++dir) {
            mTeamTextures[team][dir].loadFromFile(base + paths[team] + files[dir]);
        }
    }

    // 3. Initialize Player Sprite - Emplace before calling ->setOrigin
    mPlayerSprite.emplace(mTeamTextures[0][0]);
    mPlayerSprite->setOrigin({ 12.f, 12.f });
}

void Renderer::render(sf::RenderWindow& window, const GameStatePacket& gameState) {
    // --- 1. SET VIEW TO HUD/BACKGROUND ---
    window.setView(window.getDefaultView());

    // A. Draw Background (Grass)
    if (mBackgroundSprite.has_value()) {
        window.draw(*mBackgroundSprite);
    }

    // Optional Decoration (Safety check added)
    if (mDecorationSprite.has_value()) {
        window.draw(*mDecorationSprite);
    }

    // --- 2. SWITCH TO WORLD VIEW (Gameplay) ---
    sf::View worldView(gameState.pitchBounds);
    worldView.setViewport(sf::FloatRect({ 0.f, 0.f }, { 1.f, 1.f }));
    window.setView(worldView);

    // B. Draw Field Markings (Center Circle and Penalty Boxes)
    // We draw these BEFORE the ball and players so they are on the "floor"
    if (mCenterCircle.has_value()) {
        mCenterCircle->setPosition({ gameState.pitchBounds.size.x / 2.f, gameState.pitchBounds.size.y / 2.f });
        window.draw(*mCenterCircle);
    }

    if (mLeftPenalty.has_value()) {
        // Aligned to left edge, vertically centered
        mLeftPenalty->setPosition({ 0.f, gameState.pitchBounds.size.y / 2.f });
        window.draw(*mLeftPenalty);
    }

    // C. Draw Ball
    sf::CircleShape ball(Config::BALL_RADIUS);
    ball.setOrigin({ Config::BALL_RADIUS, Config::BALL_RADIUS });
    ball.setPosition(gameState.ballPosition);
    ball.setFillColor(sf::Color::White);
    window.draw(ball);

    // D. Draw Animated Players
    if (mPlayerSprite.has_value()) { // CRITICAL SAFETY CHECK
        int currentFrame = static_cast<int>(gameState.matchTimerSec / ANIM_SPEED) % 4;

        for (const auto& p : gameState.players) {
            int dir = 0;
            if (std::abs(p.facingDirection.y) > std::abs(p.facingDirection.x)) {
                dir = (p.facingDirection.y > 0) ? 0 : 1;
            }
            else {
                dir = (p.facingDirection.x > 0) ? 3 : 2;
            }

            // In SFML 3.0, make sure textures are valid before setting
            mPlayerSprite->setTexture(mTeamTextures[p.teamId][dir]);
            mPlayerSprite->setTextureRect(sf::IntRect({ currentFrame * FRAME_WIDTH, 0 }, { FRAME_WIDTH, FRAME_HEIGHT }));
            mPlayerSprite->setPosition(p.position);

            float scale = (Config::PLAYER_HALF_SIZE * 2.2f) / FRAME_WIDTH;
            mPlayerSprite->setScale({ scale, scale });

            window.draw(*mPlayerSprite);
        }
    }

    // --- 3. DRAW HUD ---
    window.setView(window.getDefaultView());
    renderHUD(window, (int)gameState.scoreHome, (int)gameState.scoreAway, gameState.matchTimerSec, (int)gameState.currentState);
}

void Renderer::renderHUD(sf::RenderWindow& window, int hScore, int aScore, float time, int state) {
    const sf::Font* font = getHudFont();
    if (!font) return;

    auto drawText = [&](const std::string& str, sf::Vector2f pos, sf::Color color, int size) {
        sf::Text text(*font, str, size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(2.f);
        text.setPosition(pos);
        window.draw(text);
        };

    drawText("HOME " + std::to_string(hScore) + " - " + std::to_string(aScore) + " AWAY", { 20, 20 }, sf::Color::White, 24);
    drawText("TIME: " + formatClock(time), { 20, 50 }, sf::Color::White, 24);
    drawText(stateNameFromId(state), { (float)Config::WINDOW_WIDTH / 2 - 50, 20 }, sf::Color::Yellow, 30);
}