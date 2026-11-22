# ADR-0001: Arena Cover and Elite Zombie Variants

## Status

Accepted

## Date

2026-11-21

## Last Verified

2026-11-22

## Decision Makers

Gameplay programmer, technical director (user-approved direction), QA lead

## Summary

Open-arena combat gives every zombie a straight-line approach and every shot a
clear lane, so positioning barely matters after the first wave. This ADR adds
deterministic per-wave cover that blocks players, zombies and projectiles, and
elite zombie variants that create priority targets from wave 4 onward.

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | raylib 6.2 |
| **Domain** | Core / Physics |
| **Knowledge Risk** | LOW — 2D rectangles, `CheckCollisionRecs`, `CheckCollisionCircleRec` are stable long-standing APIs |
| **References Consulted** | `third_party/raylib/include/raylib.h`, `docs/engine-reference/raylib/` |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Collision and resolution behaviour is covered by `logic_tests`; rendering is covered by the smoke screenshots |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | None |
| **Enables** | Future pathfinding, destructible cover, and cover-aware AI decisions |
| **Blocks** | None |
| **Ordering Note** | Cover must be generated before pickups and zombies are spawned for a wave |

## Context

### Problem Statement

Zombies chase the player in a straight per-axis line and bullets travel a clear
lane, so cover and threat prioritisation do not exist. Every wave plays the same
way: back away and fire. The game needs positional decisions without adding a
navigation library or a new runtime dependency.

### Current State

`Arena` generates only floor tiles. `Player`, `Zombie` and `Bullet` collide with
each other and the arena bounds. All enemies share one stat table and one visual
tint per type.

### Constraints

- C++17 and raylib only; no new runtime dependencies (project rule).
- Deterministic under a fixed seed; the smoke suite depends on it.
- Runs and tests must stay reproducible; cover layout cannot use wall-clock RNG.
- Window-free `logic_tests` must exercise the collision rules.
- The player spawn and a firing lane around it must stay clear so a run cannot
  start inside a wall.

### Requirements

- Two to twelve static cover rectangles per wave (arena-capped), deterministic
  for a seed.
- Cover blocks the player, zombies, player bullets and spitter projectiles.
- Zombies must not stall permanently on a flat wall face when a path around it
  exists via axis sliding plus a bounded detour (0.5 s blocked, then steer along
  the face toward the nearer cover edge).
- Pickups and zombie spawns must not appear inside cover. Spawns are 60 px
  inside the playable rectangle, at least 40 px apart, and at least 180 px from
  a living player.
- From wave 4, each wave contains at least one elite; later waves add random
  elites. Elites are visually distinct and worth more.
- No measurable frame-time regression at the existing entity counts.

## Decision

1. `Arena` owns a `std::vector<Rectangle>` of cover, rebuilt in `SetWave` from a
   dedicated `Rng` seeded with the run seed. Placement is tile-aligned (50 px),
   inset **three tiles (150 px)** from the wall, spaced **150 px** apart (wider
   than the boss body: the 75 px bloater sprite at 2.2 scale has a ~132 px
   collision diameter), and rejected within 160 px of the arena centre. Count is `min(2 + wave / 3, 12)`. Arena side is capped at 1800 px so
   the cover budget and floor draw stay bounded.
2. `ObstacleCollision.h` provides `CircleOverlapsRect`, `ResolveCircleVsRect`,
   `ResolveCircleAgainstObstacles` (two passes for corners) and
   `CircleBlockedByObstacles`. All movers share these rules.
3. The player and zombies resolve out of cover after each movement step. Zombie
   chase uses per-axis sliding, but axis sliding alone stalls on a single
   convex crate: after 0.5 s blocked, the zombie steers along the face toward
   the nearer cover edge. This bounded detour replaces full pathfinding. Every
   mover is also clamped to the playable rectangle.
4. Bullets sweep their frame segment, including the segment that exhausts
   range, against cover and zombies; the nearest impact wins. A wall in front
   of a zombie absorbs the shot and raises a spark. Spitter projectiles are
   consumed on cover, and spitters fire only with line of sight.
5. Pickups reject blocked spawn points and resolve forced boss/elite drops out
   of cover.
6. `Zombie` gains an `elite` flag: +2x health, 1.15x speed, 1.25x scale, 1.5x
   contact damage, triple score and scrap, a white rim/crown that multiplies the
   type tint (never replaces it), and a guaranteed health or ammo drop on
   death. Bosses are never elite. Wave assignment lives
   in `Game::BeginWave` and uses the existing spawn RNG.

### Architecture

```
Arena::SetWave -> BuildObstacles -> Arena::Obstacles()
                                      |
        +-----------------------------+-----------------------------+
        |                             |                             |
  Player::Update              Zombie::Update                Game::UpdateCollisions
  (resolve circle)            (resolve per axis)            (segment vs cover)
        |                             |                             |
        +--> ObstacleCollision.h <----+-----------------------------+
                                      |
                              Pickup::Spawn / ForceSpawn
```

### Key Interfaces

```cpp
const std::vector<Rectangle> &Arena::Obstacles() const;
void Arena::DrawObstacles() const;

bool CircleOverlapsRect(Vector2 center, float radius, Rectangle rect);
bool ResolveCircleVsRect(Vector2 &center, float radius, Rectangle rect);
void ResolveCircleAgainstObstacles(Vector2 &center, float radius,
                                   const std::vector<Rectangle> &obstacles);
bool CircleBlockedByObstacles(Vector2 center, float radius,
                              const std::vector<Rectangle> &obstacles);

void Player::Update(float dt, Vector2 move, Vector2 aim,
                    const std::vector<Rectangle> *obstacles = nullptr);
void Zombie::Update(float dt, Vector2 target,
                    const std::vector<Rectangle> *obstacles = nullptr);
void Pickup::SetObstacles(const std::vector<Rectangle> *obstacles);
```

### Implementation Guidelines

- Never mutate the obstacle vector during a wave; rebuild the whole set in
  `SetWave`. `Pickup` stores a pointer to the vector, not a copy.
- Keep the centre-clear rejection in `BuildObstacles`; removing it breaks the
  spawn guarantee and the smoke test.
- Resolve after movement, then clamp to the arena, so cover cannot push an
  entity through the outer wall.
- Elite stat changes belong in `Zombie::Spawn`; `Game` only decides which
  zombies are elite and pays the rewards.

## Alternatives Considered

### Alternative 1: Tile-grid wall map with A* pathfinding

- **Description**: Mark tiles solid in the arena grid and run A* per zombie.
- **Pros**: Clean pathing around U-shaped cover; supports maze layouts.
- **Cons**: New navigation subsystem, per-frame path cost, more state and tests;
  far more than the current straight-chase AI needs.
- **Estimated Effort**: 3-4x the chosen approach.
- **Rejection Reason**: Axis sliding is adequate for isolated cover rectangles.

### Alternative 2: Elite zombies as new `ZombieType` enum entries

- **Description**: Add `EliteChaser`, `EliteBloater`, etc.
- **Pros**: Explicit stats per type.
- **Cons**: Combines type identity with a modifier, duplicates every stat
  switch, and makes future modifiers (armoured, burning) combinatorial.
- **Estimated Effort**: Similar for this one modifier.
- **Rejection Reason**: A boolean modifier scales to future variants.

## Consequences

### Positive

- Positioning and cover use become part of combat; spitters and elites create
  target-priority decisions.
- Cover and elites are deterministic, so runs and the smoke suite stay
  reproducible.
- One shared collision helper keeps player, zombie, bullet and pickup rules in
  sync.

### Negative

- Zombies can still catch on concave cover arrangements; only isolated
  rectangles are generated, and the two-pass resolver mitigates corners.
- Fast bullets hitting cover lose the damage instead of penetrating, which can
  feel unfair when a zombie hugs a wall.
- Elite health inflation can slow early wave-4 clears if the elite is a bloater.

### Neutral

- Cover is drawn with primitive shapes rather than a new tilesheet asset.
- `Player::Update`, `Zombie::Update` and `Pickup` gained optional/pointer
  parameters, so existing call sites and tests remain valid.

## Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|-----------|
| Cover blocks the smoke bot's line of fire | Medium | Test flake | 160 px centre-clear zone keeps the spawn lane open; smoke bot fights at close range |
| Zombie stuck on a wall or crate | Medium | Gameplay stall / safe pocket | Per-axis sliding plus a 0.5 s blocked detour; two-pass corner resolution; logic test must cover an isolated crate |
| Boss walled off by cover | Medium | Boss pillar fails | 150 px inset/spacing exceeds the ~132 px boss body; add a boss-through-gap logic test |
| Pickup lands inside cover | Low | Unreachable drop | Spawn rejection plus forced-drop resolution |
| Elite spike too harsh | Medium | Difficulty | One guaranteed elite from wave 4; random extras capped at 25% |

## Performance Implications

| Metric | Before | Expected After | Budget |
|--------|--------|---------------|--------|
| CPU (frame time) | < 1 ms | < 1 ms at 12 obstacles + capped floor | 16.6 ms |
| Memory | negligible | < 1 KB of rectangles and pickups | 1 MB |
| Load Time | unchanged | unchanged | — |
| Network | N/A | N/A | — |

## Migration Plan

1. Add `ObstacleCollision.h` and arena generation; verify deterministic layout
   and spawn clearance in `logic_tests`.
2. Pass the obstacle list into `Player` and `Zombie`; verify blocked movement
   and wall sliding in `logic_tests`.
3. Add bullet/projectile blocking, pickup avoidance and elite assignment; run
   the smoke suite and inspect wave screenshots.
4. Roll out to the full wave loop; keep the old call signatures valid via
   default arguments.

**Rollback plan**: Remove the `BuildObstacles` call and pass `nullptr`/skip the
obstacle loops; elites can be disabled by forcing `elite = false` in
`BeginWave`. The gameplay otherwise remains the previous straight-chase game.

## Validation Criteria

- [x] Wave 1 has at least two cover rectangles and wave 10 no more than twelve.
- [x] Cover is deterministic for a seed and never within 160 px of the centre.
- [x] Player and zombie positions never overlap cover after movement.
- [x] A swept bullet stops at cover before a zombie behind it.
- [x] Pickups never spawn inside cover.
- [x] Wave 5 contains at least one elite; elites have 2x health and 3x rewards.
- [x] `ctest` passes 2/2 (assertion counts are tracked by the test run, not this ADR).

## GDD Requirements Addressed

This decision is reflected in `combat-system.md` and `wave-system.md` (revised
2026-11-22). Enables: cover-aware combat, elite threat prioritisation, and
future destructible cover or navigation systems.

## Related

- `docs/raylib-port.md` — feature set, layout and verification.
- `src/Arena.{h,cpp}` — deterministic cover generation and rendering.
- `src/ObstacleCollision.h` — shared circle/segment cover rules.
- `src/Player.cpp`, `src/Zombie.cpp`, `src/Pickup.cpp`, `src/Game.cpp`.
- Tests: `tests/logic_tests.cpp`, `src/SmokeTest.cpp`.

---

## Revision Note — 2026-11-22

The per-GDD design review revised the wave/combat contract: cover count cap
12, arena cap 1800 px, 60 px spawn inset with 40 px spacing and 180 px player
distance, a 0.5 s blocked detour, spitter line-of-sight, terminal bullet sweep,
and elite tint multiplication. The cover inset and spacing are now **150 px**
so the ~132 px boss body can pass; the 0.5 s detour prevents a single crate from
becoming a safe pocket. The decision text above reflects the revised contract;
the implementation deltas are tracked in `wave-system.md` and
`combat-system.md`.
