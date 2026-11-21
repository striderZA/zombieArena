# Technical Preferences

<!-- Project-specific standards referenced by OCGS agents. -->

## Engine & Language

- **Engine**: Raylib 6.2 (vendored at `third_party/raylib`)
- **Language**: C++17
- **Rendering**: raylib 2D (`Camera2D`, textures, shapes); 1280x720 reference
  layout scaled by window height, optional fullscreen
- **Physics**: Custom AABB and circle tests; no third-party physics engine

## Input & Platform

- **Target Platforms**: Windows desktop (current build); CMake path exists for
  macOS/Linux with a system raylib
- **Input Methods**: Keyboard/mouse and gamepad
- **Primary Input**: Keyboard + mouse for player 1
- **Gamepad Support**: Full (player 1 in single-player, player 2 in co-op)
- **Touch Support**: None
- **Platform Notes**: The MinGW-w64 build is static; raylib is vendored for
  Windows. UI uses a pixel TTF font and scales with window height.

## Naming Conventions

- **Classes / Structs**: PascalCase (`Game`, `PlayerSlot`, `EnemyShot`)
- **Functions / Methods**: PascalCase (`UpdatePlaying`, `FireWeapon`)
- **Variables**: `camelCase`; member data uses a trailing underscore (`state_`)
- **Constants**: `kPascalCase` (`kMaxBullets`, `kPlayerHitShake`)
- **Files**: PascalCase for source, lowercase for docs and scripts
- **Scenes / Prefabs**: Not applicable (code-driven raylib game)
- **Enums**: PascalCase enum names and enumerators (`GameState::Playing`)

## Performance Budgets

- **Target Framerate**: 60 FPS
- **Frame Budget**: 16.67 ms
- **Draw Calls**: Not yet instrumented; keep per-frame arena/texture draws
  bounded and reuse loaded assets
- **Memory Ceiling**: Not yet measured; assets are ~14 MB and loaded once.
  Avoid per-frame allocations in hot paths.

## Testing

- **Framework**: CTest; custom assertion runner in `tests/logic_tests.cpp`;
  scripted integration/screenshot suite in `src/SmokeTest.cpp`
- **Minimum Coverage**: All deterministic gameplay rules and any new formula
  need logic assertions; any state/UI/weapon/co-op flow change needs a smoke
  assertion or screenshot evidence
- **Required Tests**: Player movement/collision, weapons and reloads, wave
  composition, meta-profile persistence, settings, co-op, boss behavior, and
  bullet swept collision

## Forbidden Patterns

- No runtime dependency beyond raylib and the C++17 standard library without
  an ADR
- No blocking file/network I/O in the frame loop
- No hardcoded absolute asset paths; resolve assets relative to the executable
  or working directory
- Do not weaken or delete existing tests to make a change pass

## Allowed Libraries / Addons

- raylib 6.2 (core, raudio, raymath; no raygui currently)
- C++17 standard library
- CMake 3.20+ build tooling

## Architecture Decisions Log

- [No formal ADRs yet — add under `docs/architecture/` when a system changes]

## Engine Specialists

- **Primary**: `raylib-specialist`
- **Language/Code Specialist**: `raylib-specialist` (C++17)
- **Shader Specialist**: `raylib-specialist` (no custom shaders currently)
- **UI Specialist**: `raylib-specialist` (immediate-mode drawing)
- **Additional Specialists**: None installed beyond the cross-cutting
  `technical-artist` and `sound-designer` used by team skills
- **Routing Notes**: Route raylib API, CMake, asset loading, and C++ game code
  questions to `raylib-specialist`; use `technical-artist` for rendering/VFX
  pipeline questions and `sound-designer` for audio-event specs.

### File Extension Routing

| File Extension / Type | Specialist to Spawn |
|-----------------------|---------------------|
| `.cpp`, `.h`, `.hpp` (game code) | `raylib-specialist` |
| Shader / material files | `raylib-specialist` |
| UI / screen drawing code | `raylib-specialist` |
| Scene / level data | `raylib-specialist` |
| Native extension / plugin files | `raylib-specialist` |
| General architecture review | `technical-director` |

## Project Notes

- Existing gameplay constants predate OCGS. New tuning should be data-driven;
  migrate old constants only in focused changes with tests.
- The smoke suite is deterministic under a fixed seed and is the primary
  end-to-end verification path.
- Bullet collisions use swept segment-vs-rect tests; preserve that behavior.
