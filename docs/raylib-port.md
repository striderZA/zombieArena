# Zombie Arena — raylib rebuild

A raylib rebuild of the SFML "Zombie Arena" game from *Beginning C++ Game
Programming*. The original SFML reference project was removed when the repo
adopted the OCGS layout; it remains in git history. All paths below are
relative to the repository root.

Top-down wave shooter with four weapons, seven enemy types including a boss,
randomized upgrade drafts, persistent scrap and permanent upgrades, local
two-player co-op, gamepad support, and a settings/bunker layer.

## Build

Prerequisites:

- CMake 3.20+
- MinGW-w64 GCC with C++17 (tested with GCC 15.2.0)
- Windows (raylib 6.2 prebuilt for MinGW is vendored in `third_party/raylib`)

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ZombieArenaRaylib.exe
```

The executable is fully static (only Windows system DLLs are imported) and the
build step copies `assets/` next to it. The game finds `assets/` relative to the
working directory or the executable, so it runs from either the build tree or
the source tree.

On other platforms install raylib (e.g. `find_package(raylib)`) and configure
with `-DRAYLIB_ROOT=/path/to/raylib`.

## Controls

| Input | Action |
|---|---|
| WASD | Move player 1 (rebindable) |
| Mouse | Aim player 1 |
| Left mouse | Fire player 1 |
| R | Reload (rebindable) |
| 1 / 2 / 3 / 4 | Pistol / shotgun / rifle / SMG |
| Mouse wheel | Cycle unlocked weapons |
| 1 / 2 / 3 | Pick an upgrade card between waves |
| Enter / Space | Start, pause/resume, restart |
| M | Bunker (title / game-over screen) |
| S | Settings (title / pause / game-over) |
| C | Toggle co-op (title, requires a gamepad) |
| F11 | Toggle fullscreen |
| Esc | Quit |

Gamepad:

| Input | Action |
|---|---|
| Left stick | Move |
| Right stick | Aim |
| RT | Fire |
| X | Reload |
| LB / RB | Cycle weapons |
| A / B / X | Draft card 1 / 2 / 3, confirm / back |
| Start | Start, pause/resume |
| D-pad | Menu navigation |

In single-player a gamepad drives player 1. In co-op, player 1 keeps
keyboard/mouse and player 2 uses the gamepad.

## Run options

```text
--windowed [WxH]    start in a window (configured size when WxH is omitted)
--fullscreen        start fullscreen
--seed N            reproduce a launch (default: random per launch)
--coop              enable co-op (player 2 needs a gamepad)
--no-coop           disable co-op
--wave N            quick-start on wave N (QA/demo; skips the first draft)
--profile PATH      profile file (default: assets/gamedata/profile.txt)
--settings PATH     settings file (default: assets/gamedata/settings.txt)
--smoke-test        scripted integration test, screenshots in smoke/
--screenshots DIR   screenshot output directory for --smoke-test
--help
```

## Progression, settings and saves

- **Scrap** is earned per kill and per cleared wave. The **Bunker** spends it on
  five permanent upgrades (max health, bullet damage, move speed, ammo pickup
  value, scrap gain) and a one-time **SMG blueprint** unlock.
- Stats persist: runs, kills, best wave and lifetime scrap.
- **Settings** persist master/music/SFX volume, fullscreen, windowed resolution,
  screen shake, co-op and rebindable movement/reload/pause keys.
- Save files (created on demand):
  - `assets/gamedata/scores.txt` — high score
  - `assets/gamedata/profile.txt` — scrap, permanent upgrades, stats
  - `assets/gamedata/settings.txt` — settings and key binds

## Feature set

- **Weapons** — pistol (1/s, 6/48), shotgun (6 pellets, 2/12), rifle (4/s,
  20/90), SMG (8/s, 30/120; unlockable). Independent ammo pools, pickup shares,
  spread, reload and HUD.
- **Enemies** — bloater, chaser, crawler, ranged spitter, fast sprinter,
  exploder (death blast damages players), and a boss every five waves with a
  boss health bar, large hitbox, heavy knockback resistance and bonus rewards.
- **Upgrade drafts** — three weighted cards per level-up from 11 upgrades plus a
  repair fallback; common/rare/epic rarity, stack caps, and effects on both
  players in co-op.
- **Co-op** — two players, shared score/scrap, separate health, ammo and
  weapons, nearest-player enemy targeting, midpoint camera, player 2 crosshair
  and HUD, revive at 50% health at the next wave, game over only when both are
  down.
- **Feedback** — screen shake, muzzle flash, hit flash and knockback, damage
  numbers, persistent blood decals, spawn rings, wave banners, boss bar,
  explosion shockwaves and a damage vignette.
- **Menus** — title with profile stats, bunker shop, settings, controls
  rebinding, pause and game-over screens.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

- `logic_tests` — window-free checks for weapon pools and unlocks, upgrade
  math, zombie stats/attacks/bosses, wave composition, pickup timers, player
  damage/revive/meta math, arena growth, profile and settings serialization, key
  names and RNG.
- `smoke` — drives the real game loop: title → draft → weapon swaps and shotgun
  spread → wave 1 combat → pause/resume → wave clear → death/restart; settings,
  key rebinding and bunker purchases; two-player co-op movement/fire/death; and
  a wave-5 quick start with boss, spitters and projectiles. It also checks every
  captured screenshot contains rendered pixels.

## Layout

```text
src/
  main.cpp        window/input/gamepad/CLI
  Game.cpp        states, waves, co-op, meta, settings UI, HUD, audio
  Player.cpp      movement, aim, health, revive, meta scaling
  Zombie.cpp      seven enemy types, behaviours, boss scaling
  Bullet.cpp      projectile pool with spread/damage
  Pickup.cpp      timed health/ammo pickups, boss drops
  Arena.cpp       tiled arena generated per wave
  Waves.cpp       wave counts and weighted enemy composition
  Weapons.cpp     four weapons, ammo pools, unlocks and multipliers
  Upgrades.cpp    upgrade card table (rarity, weights, stack caps)
  Meta.cpp        profile, permanent upgrades and costs
  Settings.cpp    settings, key binds and serialization
  Assets.cpp      texture/font/sound/music loading and asset lookup
  SmokeTest.cpp   scripted integration test
tests/logic_tests.cpp  window-free simulation tests
third_party/raylib/  raylib 6.2 headers + static MinGW library
assets/             sprites, tilesheet, font, sounds, high score
```

## Intentional changes from the SFML original

Behavior is otherwise preserved (arena growth, base zombie stats, base
ammo/health values, 250 ms hit cooldown, 1000 px bullet range).

- Aim rotation is relative to the player, not the screen centre.
- Every zombie receives its per-wave speed bonus; the original applied the
  three upgrades to the first zombie only.
- Explicit state transitions for pause, wave clear and game over.
- Music switches per state and stops on death.
- Deterministic RNG: `--seed` fixes the launch seed, and each in-session run
  mixes a run counter so hordes/drafts differ between attempts.
- Bullets use a normalized direction (constant 1000 px/s) instead of the
  original gradient formula, which made diagonal shots slower.
- Pickup timers are initialized and pickups spawn fully inside the arena.
- `graphics/background3.png` was actually JPEG data; it is stored as a real PNG
  because raylib selects its image decoder by file extension.
- HUD scales with window height instead of fixed pixel positions.

## Verification

`ctest` result: 2/2 suites pass — **656 logic assertions** and **96 integration
checks**. The smoke run includes a five-zombie wave-1 clear (score 50), wave-2
death/restart, settings/keybinding and bunker/SMG purchase paths, a two-player
co-op wave and double-down game over, and a wave-5 boss with spitters and live
projectiles, plus a 0.1 s / 100 px swept bullet hit that endpoint-only
collision misses. Windowed and fullscreen launches were also run directly.

Gamepad input paths are exercised through scripted player-2 inputs; no physical
gamepad was available in this environment.
