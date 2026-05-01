# Super Copa Perú Evolution

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.md)
[![SFML](https://img.shields.io/badge/SFML-3.1-green.svg)](https://www.sfml-dev.org/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Build with CMake](https://img.shields.io/badge/CMake-3.15+-064F8C.svg)](https://cmake.org/)

🇵🇪⚽

> *Arcade football with Peruvian chaos, wild momentum, and the occasional cow on the pitch.*

**Super Copa Perú Evolution** is a C++ / SFML arcade football game inspired by the spirit of **Copa Perú** — Peru’s famously chaotic, regional, anything-can-happen football competition.

The game leans into fast action, funny moments, and sudden momentum swings. Matches are built around possession, pressure, and unpredictable chaos, with the legendary **cow invasion event** setting the tone.

---

## What Is Copa Perú?

Copa Perú is a real football tournament in Peru known for regional identity, passion, and unpredictability. It is the perfect inspiration for a game that feels:

**fast** · **chaotic** · **funny** · **dramatic**

---

## Quick Start: How to Play

**Goal:** Score more goals than the opponent in 3 minutes.

| Action | Default Key |
|--------|-------------|
| Move player | `WASD` |
| Pass | `J` |
| Shoot | `K` |
| Tackle / Steal | `L` |
| Switch player (defense) | `I` |

- Possession changes automatically after tackles or goals.
- The cow invasion triggers randomly.
- Cows block shots, disrupt passes, and cannot leave the pitch.

---

## Gameplay

Super Copa Perú Evolution is built around quick, readable football action:

- Move, pass, shoot, tackle, and switch players on defense
- AI teammates and opponents pressure the ball and react intelligently
- Goalkeepers protect the six-yard box
- Host-authoritative simulation keeps match logic consistent
- Survive chaos events like the **cow invasion**

### What Makes It Different?

This is **not** a realistic football sim. It is an arcade experience with:

- fast ball movement
- possession-based control
- defensive switching
- absurd match events that make every game feel unique

---

## Features

### Core Football
- Player movement and ball control
- Passing and shooting
- Tackling and steals
- Player switching on defense
- Goal detection and scoring
- Kickoff and restart flow
- Match timer and state handling

### AI and Simulation
- AI teammates and opponents
- Defensive and attacking behavior
- Separation physics to avoid player overlap
- Goalkeeper logic
- Six-yard box protection
- Host-authoritative architecture

### Copa Perú Chaos
- **Cow invasion event** — cows move onto the pitch and interfere with play
- Cows do not clip into goals or leave the pitch incorrectly
- Designed for memorable, chaotic moments

### Presentation
- SFML-based 2D rendering
- Player and team textures
- Directional animation support
- Team uniforms
- Arcade-style visual presentation

---

## Download and Build


Requirements:
- CMake 3.15+
- A C++17 compiler
- Git
- SFML 3.1 (fetched automatically by CMake)

```bash
git clone https://github.com/blayne-04/SPCE.git
cd SPCE
cmake -S . -B build
cmake --build build
```

The executable will be created in:

```bash
build/bin/
```

### Run the Game

Make sure your working directory is the project root, then launch the executable from `build/bin/`.

---

## Project Structure

```text
src/
├── Common/     shared constants, packets, and types
├── Core/       renderer and core engine systems
├── Input/      player and AI input handling
├── Objects/    players, ball, goals, and related entities
└── Simulation/ world logic, match rules, physics, and AI behavior
```

---

## Development Team

Alphabetical order:

- **Blayne Fuller**
- **Brian Reano Juarez**
- **Ryan Von Bereghy**
- **Santiago Pelaez**

---

## License

This project is licensed under the MIT License. See [`LICENSE.md`](LICENSE.md) for details.
