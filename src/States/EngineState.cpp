#include "EngineState.h"
#include "../Core/GameEngine.h"
#include "../Common/Constants.h"
#include "../Input/AiController.h"

#include <memory>

namespace {
	void drawSolidBackground(sf::RenderWindow& window, sf::Color color) {
		sf::RectangleShape background(sf::Vector2f(
			static_cast<float>(Config::WINDOW_WIDTH),
			static_cast<float>(Config::WINDOW_HEIGHT)
		));
		background.setFillColor(color);
		window.draw(background);
	}
} // namespace

// ============================================================================
// StartMenuState
// ============================================================================

void StartMenuState::handleInput(GameEngine& engine, sf::Event& event) {
	const auto* key = event.getIf<sf::Event::KeyPressed>();
	if (!key) return;

	if (key->code == sf::Keyboard::Key::H) {
		engine.transitionTo(std::make_unique<HostPlayingState>());
		return;
	}

	if (key->code == sf::Keyboard::Key::C) {
		engine.transitionTo(std::make_unique<ClientPlayingState>());
		return;
	}
}

void StartMenuState::tick(GameEngine&, float) {}

void StartMenuState::render(GameEngine& engine) {
	drawSolidBackground(engine.getWindow(), sf::Color(50, 50, 100));
}

// ============================================================================
// SettingsMenuState (placeholder)
// ============================================================================

void SettingsMenuState::handleInput(GameEngine&, sf::Event&) {}
void SettingsMenuState::tick(GameEngine&, float) {}

void SettingsMenuState::render(GameEngine& engine) {
	drawSolidBackground(engine.getWindow(), sf::Color(50, 100, 50));
}

// ============================================================================
// PauseMenuState (placeholder)
// ============================================================================

void PauseMenuState::handleInput(GameEngine&, sf::Event&) {}
void PauseMenuState::tick(GameEngine&, float) {}

void PauseMenuState::render(GameEngine& engine) {
	// Semi-transparent overlay. GameEngine renders states bottom-to-top.
	drawSolidBackground(engine.getWindow(), sf::Color(0, 0, 0, 180));
}

// ============================================================================
// ClientPlayingState (placeholder)
// ============================================================================

void ClientPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	const auto* key = event.getIf<sf::Event::KeyPressed>();
	if (!key) return;

	if (key->code == sf::Keyboard::Key::P) {
		engine.pushState(std::make_unique<PauseMenuState>());
		return;
	}
}

void ClientPlayingState::tick(GameEngine& engine, float dt) {
	auto& net = engine.getNetwork();

	// Start client socket once (binds AnyPort, stores host ip:port).
	if (!mStarted) {
		net.startClient(sf::IpAddress::LocalHost, Config::HOST_PORT);
		mStarted = true;

		mHaveState = false;
		mNextInputSequence = 0;
		mLastState = GameStatePacket{};
	}

	// If we haven't received a snapshot, we don't know controlledAwayPlayerId yet.
	constexpr std::uint8_t kFallbackAwayPlayerId = 4;
	const std::uint8_t playerId =
		mHaveState ? mLastState.controlledAwayPlayerId :
		kFallbackAwayPlayerId;

	InputPacket input{};

	if (mHaveState) {
		// Normal: send keyboard DOWN-state every tick.
		input = mInput.getLocalInput(playerId);
	}
	else {
		// Handshake: send "neutral" INPUT so host learns our (ip, port).
		input.playerId = playerId;
		input.moveDirection = sf::Vector2f(0.f, 0.f);
		input.shootDown = false;
		input.passDown = false;
		input.tackleDown = false;
		input.switchDown = false;
		input.lungeDown = false;
	}

	// Always stamp routing + sequencing last.
	input.playerId = playerId;
	input.inputSequence = mNextInputSequence++;

	net.sendPlayerInput(input);

	// Drain socket; keep the newest STATE by frameNumber.
	if (net.receiveLatestGameState(mLastState)) {
		mHaveState = true;
	}
}

void ClientPlayingState::render(GameEngine& engine) {
	mRenderer.render(engine.getWindow(), mLastState);
}


// ============================================================================
// HostPlayingState (placeholder)
// ============================================================================

void HostPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	const auto* key = event.getIf<sf::Event::KeyPressed>();
	if (!key) return;

	if (key->code == sf::Keyboard::Key::P) {
		engine.pushState(std::make_unique<PauseMenuState>());
		return;
	}
}

void HostPlayingState::tick(GameEngine& engine, float dt) {
	auto& net = engine.getNetwork();
	auto& match = engine.getMatch();

	if (!mStarted) {
		net.startHost(Config::HOST_PORT);
		mStarted = true;

		// SANTI: for now we hard-reserve player 0 (host) and player 4 (client).
		match.setControlledPlayerIds(0, 4);
	}

	// SANTI: snapshot at the START of the tick for AI decisions.
	// AI reads the same data contract as networking/rendering (GameStatePacket).
	GameStatePacket snapshotBefore{};
	match.getGameState(snapshotBefore);

	FrameInput frame{};

	// SANTI: track which player slots are human-controlled this tick.
	std::array<bool, Config::kNumPlayers> isHuman{};
	isHuman.fill(false);

	// Host human
	isHuman[0] = true;

	// SANTI: reserve client slot so AI won't move it before first INPUT arrives.
	isHuman[4] = true;

	// Local host controls player 0.
	frame.inputs[0] = mInput.getLocalInput(0);
	frame.inputs[0].playerId = 0;

	// Drain incoming client INPUT packets.
	std::queue<InputPacket> incoming;
	net.pollIncomingInputs(incoming);

	while (!incoming.empty()) {
		const InputPacket p = incoming.front();
		incoming.pop();

		if (p.playerId >= Config::kNumPlayers) continue;

		frame.inputs[p.playerId] = p;
		isHuman[p.playerId] = true;
	}

	// SANTI: fill the remaining slots with AI-generated inputs.
	for (std::size_t i = 0; i < Config::kNumPlayers; ++i) {
		if (isHuman[i]) continue;

		frame.inputs[i] = mAi.getAIInput(static_cast<std::uint8_t>(i), snapshotBefore);
		frame.inputs[i].playerId = static_cast<std::uint8_t>(i);
	}

	// Referee step.
	match.update(frame, dt);

	// Publish snapshot for render + networking.
	match.getGameState(mLastState);
	net.sendGameState(mLastState);
}


void HostPlayingState::render(GameEngine& engine) {
	mRenderer.render(engine.getWindow(), mLastState);
}

// ============================================================================
// SinglePlayerPlayingState (placeholder)
// ============================================================================

void SinglePlayerPlayingState::handleInput(GameEngine& engine, sf::Event& event) {
	const auto* key = event.getIf<sf::Event::KeyPressed>();
	if (!key) return;

	if (key->code == sf::Keyboard::Key::P) {
		engine.pushState(std::make_unique<PauseMenuState>());
		return;
	}
}

void SinglePlayerPlayingState::tick(GameEngine&, float) {}

void SinglePlayerPlayingState::render(GameEngine& engine) {
	drawSolidBackground(engine.getWindow(), sf::Color::Green);
}

