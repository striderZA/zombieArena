# Zombie Arena

> A raylib 2D top-down wave shooter: four weapons, seven enemy types, randomized
> upgrade drafts, persistent scrap, local co-op, gamepad support, and a boss
> every five waves.

Built with **C++17 + raylib 6.2** and organized with
[OpenCode Game Studios](https://github.com/striderZA/OpenCodeGameStudios).
The original SFML reference implementation is no longer in the working tree; it
remains in git history.

## Quick Start

Prerequisites (Windows):

- CMake 3.20+
- Ninja
- MinGW-w64 GCC with C++17 (tested with GCC 15.2.0)

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ZombieArenaRaylib.exe
```

raylib 6.2 is vendored under `third_party/raylib`; the build copies `assets/`
next to the executable. On other platforms, install raylib and configure with
`-DRAYLIB_ROOT=/path/to/raylib`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

Two suites run:

- `logic_tests` — deterministic simulation and combat rules.
- `smoke` — scripted gameplay integration, including screenshots in
  `build/smoke/`.

## Controls

| Input | Action |
|---|---|
| WASD | Move player 1 |
| Mouse | Aim / fire |
| R | Reload |
| 1 / 2 / 3 / 4 | Pistol / shotgun / rifle / SMG |
| Mouse wheel | Cycle unlocked weapons |
| Enter / Space | Start, pause/resume, restart |
| M | Bunker (title / game over) |
| S | Settings (title / pause / game over) |
| C | Toggle co-op (requires a gamepad) |
| F11 | Toggle fullscreen |
| Esc | Quit |

Gamepad: left stick move, right stick aim, RT fire, X reload, LB/RB cycle
weapons, Start pause. In co-op, player 2 uses the gamepad.

The full control, progression, save-file, and verification reference lives in
[`docs/raylib-port.md`](docs/raylib-port.md).

## Project Layout

```text
/
├── src/                  # C++ game source
├── assets/               # PNG sprites/tiles, WAV audio, TTF fonts, save data
├── tests/                # CMake logic tests + scripted smoke suite
├── third_party/raylib/   # vendored raylib 6.2 (MinGW-w64)
├── design/               # OCGS design documents and entity registry
├── docs/                 # architecture, engine reference, framework docs
├── production/           # stage and session state
├── prototypes/           # throwaway experiments
├── .agents/              # canonical OCGS agents, skills, commands, rules
├── .omp/                 # oh-my-pi harness config and generated agents
└── build/                # generated; ignored
```

## OpenCode Game Studios

This repo uses the OCGS agent workflow with the **oh-my-pi (omp)** harness.
Installed modules:

- `core`
- `engine-raylib`
- `programming`
- `qa`
- `design`
- `architecture`

The design/QA team skills reference `technical-artist` and `sound-designer`,
so those two cross-cutting agents are installed without the full art/audio
modules. Add more modules with:

```sh
node .opencode/modules/install.mjs add <module> --harness omp
```

Start `omp` from the repository root. New agents/skills load on the next
session; use `/` or `/skill:<name>` to find installed workflows. Project
instructions are in [`AGENTS.md`](AGENTS.md), and technical conventions are in
[`docs/framework/technical-preferences.md`](docs/framework/technical-preferences.md).
The other harness configs are retained for future use; on Windows the
`.opencode/` symlink stubs may need `node tools/fix-opencode-symlinks.mjs`
before OpenCode can discover them.

## Attribution

The game design and assets originate from the SFML **Zombie Arena** sample in
*Beginning C++ Game Programming* by John Horton. The raylib port and the OCGS
project configuration are maintained in this repository. The MIT license in
[`LICENSE`](LICENSE) covers the OCGS framework files; it does not relicense the
original third-party game assets.
