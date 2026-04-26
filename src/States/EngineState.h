#pragma once

#include <SFML/Graphics.hpp>

/* Forward Declaration */
class GameEngine;

class EngineState {
public:
    virtual ~EngineState() = default;

    virtual void handleInput(GameEngine& engine, sf::Event& event) = 0;
    virtual void update(GameEngine& engine, float dt) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
};

class StartMenuState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class SettingsMenuState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class PauseMenuState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class ClientPlayingState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class HostPlayingState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};

class SinglePlayerPlayingState : public EngineState {
public:
    void handleInput(GameEngine& engine, sf::Event& event) override;
    void update(GameEngine& engine, float dt) override;
    void render(sf::RenderWindow& window) override;
};
