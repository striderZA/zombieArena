# Pickups

> **Status**: In Design
> **Author**: systems-designer + user
> **Last Updated**: 2026-11-22
> **Implements Pillar**: Escalating, readable pressure

## Overview

Pickups are the arena's supply layer: deterministic health and ammo tokens that
spawn at wave start and are forced from elites and bosses, collected by walking
over them. The system owns spawn eligibility (never inside cover, at least
180 px from a living player, 40 px apart), the lifetime/refill cadence that
makes Scavenger meaningful, and the per-weapon ammo share that keeps
one-weapon runs solvent. It exists so ammo and health are a readable, contested
resource rather than an invisible refill; without it, the threat budget's
sustained-DPS gate has no source. ADR-0001 fixes the ordering: cover is
generated before pickups, and placement rejects cover intersections.

## Player Fantasy

Wave seven. The rifle is down to four rounds and the spare is empty. The ammo
drop — the one the elite just dropped, or the one the wave laid down at the
bell — is live across the cover cluster, its timer already running. Between you
and it: a spitter's lane and the front of a chaser pack. You can spend the last
four rounds carving a path and arrive dry, or break line of sight and meet the
next pack with an empty gun. The drop is not a reward for clearing the wave; it
is the wave's next decision, lying in the open.

The feeling is held-breath greed: the dare of "I can make it," resolving into
relief or the specific sting of "that was mine to lose." A drop is a
threat-shaped opportunity, not loot. The supply layer underneath is
deterministic — one health and one ammo token at wave start, forced elite/boss
drops, one ammo type, a 6–7.8 s clock, a 4.2–6 s refill wait, and placement that
rejects cover and keeps 180 px off a living player — so the surprise is spatial,
never whether.

By wave 21 the same drop is a pace choice, not a lifeline: "is this route worth
the seconds?" The build changes the clock — Field Medic and Scavenger stretch
value and life — but the fantasy must work at Scavenger 0 first. It fails if
players say "the pickups feel rewarding." It succeeds when they narrate a verb,
a direction, and a cost: "I crossed east for the ammo and the spitter clipped
me."

## Detailed Design

### Core Rules

1. **Two types, shared tokens.** Health and ammo only. No score token, no
   temporary power-up. The boss cache is a composite token that grants both, not
   a third type.
2. **Wave-start pair.** After cover and players are placed, spawn one health and
   one ammo ambient token. They sit on opposite half-planes of the nearest
   living player, 300–700 px away, at least 35% of the arena side apart, with a
   cover-free straight corridor, 60 px cover clearance, and 40 px edge spacing.
   They are visible at the bell but not collectible, and their timers start only
   when the first zombie becomes active.
3. **Placement fallback.** If no candidate satisfies the full band, relax in
   this order: (1) remove the 700 px upper bound, keeping the 300 px minimum;
   (2) reduce the 35% separation to 0, keeping the 92 px center spacing;
   (3) drop the opposite-half-plane requirement. Never relax the 300 px minimum,
   60 px cover clearance, 180 px player clearance, 92 px spacing, or the
   inside-arena requirement. If still none, keep the token invisible and retry
   every 0.5 s; never spawn in cover or on the last rejected point.

   **Geometry definitions.** Arena side = playable width = height. Distance is
   center-to-center to the nearest living player. Separation is Euclidean
   center-to-center at least 35% of the arena side. Opposite half-planes means
   opposite sides of the reference player's vertical centerline; the reference
   is the living player nearest the pair midpoint, ties to the lowest index.
   Cover clearance is the token circle edge at least 60 px from every cover
   edge (center-to-cover at least 86 px with a 26 px radius). The corridor is
   the segment from the nearest living player's center to the token center, and
   it must intersect no cover. Edge spacing is center-to-center at least 92 px
   (26 + 26 + 40). Player clearance is at least 180 px center-to-center from
   every living player. Both tokens must be fully inside the playable
   rectangle.
4. **Per-kill salvage.** Every kill credits the killer's equipped weapon:
   `salvage_rounds = max(1, round(0.75 × max_hp / 7))`; shotgun uses
   `max(1, floor(salvage_rounds × 0.20))`. `max_hp` is the enemy's spawn max HP
   (elite doubling and boss formula included), never current HP. This is
   auto-credited, not a token. In co-op it credits the killer personally;
   ambient and forced tokens remain shared.
5. **Elite ammo.** An elite death force-spawns one ammo token at the death
   point, resolved out of cover. Value is
   `round((16 + 8 × Scavenger) × ammo_share)`; it uses a reserved forced slot, so
   it never replaces the wave-start ambient token. The newest forced token
   replaces the oldest forced token. Elite ammo is exempt from the 180 px player
   clearance (it is a kill reward), but it must be 60 px clear of cover and
   inside the arena; if the death point is invalid, relocate to the nearest valid
   point; if none exists, auto-award it.
6. **Boss cache.** A boss death force-spawns one cache token granting health
   `50 + 25 × Field Medic` (capped by max health) and ammo
   `round((16 + 8 × Scavenger) × ammo_share)` together. On collision, grant each
   component independently: health if missing, ammo if below cap. If at least
   one component is granted, consume the cache and show `FULL` or `AMMO FULL`
   for the ungranted component; if neither is eligible, do not consume it and
   show both badges. It uses its own reserved slot, is exempt from the 180 px
   player-clearance rule, and must be 60 px clear of cover; if invalid, relocate
   to the nearest valid point; if none exists, auto-award it.
7. **Collection.** Player body circle (20 px) plus token circle (26 px) overlap.
   The first eligible living player by index collects; shared, no duplicate. A
   health token is ineligible at full health and remains live, showing `FULL`;
   an ammo token is ineligible at the reserve cap and remains live. A player may
   collect one health and one ammo token in the same frame.

   **Co-op ownership.** Ambient raw value and timer use the lowest-index living
   player at wave start. Forced and cache raw values/timers use the killer's
   stacks at spawn. The ammo grant uses the collector's equipped weapon share;
   the health grant uses the token's raw value and the collector's missing
   health. Timers are token-level, not per player.
8. **Timers and refill.** Health: live `6 + 0.6 × Field Medic`, wait
   `max(1, 6 − 0.6 × Field Medic)`. Ammo: live `6 + 0.6 × Scavenger`, wait
   `max(1, 6 − 0.6 × Scavenger)`. Tokens are visible but inert at the bell; live
   timers start only when the first zombie becomes active after its 0.6 s spawn
   ring. Timers freeze during pause/draft and use strict `>` expiry with one
   transition per frame. Ambient tokens respawn after the wait at a new eligible
   position; forced tokens and the boss cache do not respawn.
9. **Late-wave floor.** From wave 20 only, the ambient ammo token is worth
   `max(base, round(0.15 × wave_round_need))` before the weapon share, where
   `wave_round_need = Σ 3 × ceil(hp_i / 21)` over the wave-start roster. Include
   elites and the boss; exclude boss phase adds. At wave 30 the expected value
   is ~165 rifle rounds (83 SMG, 33 shells); the range is 141–176 with elite
   rolls. It is still one token.
10. **Reserve cap.** 240 rifle/SMG rounds or 40 shotgun shells per player.
    Salvage and pickups above the cap are lost with an `AMMO FULL` cue; a pickup
    at cap is not consumed. The cap does not apply to health.
11. **Wave clear.** Ambient tokens despawn with `SUPPLY LOST`. Uncollected forced
    ammo and boss caches are auto-awarded to the killer on the level-up screen
    with a `SALVAGED` cue. Nothing is silently deleted.
12. **Determinism and readability.** Type and amount are fixed when the seeded
    wave plan is built (elite rolls included); only location varies with spawn
    RNG. Wave-start layout uses a per-token stream from
    `(run_seed, wave, kind, index)`; forced drops use
    `(run_seed, wave, drop_sequence)`. Fixed iteration order: health, ammo,
    players, kills. No time-based or unordered RNG.

### States and Transitions

| From | To | Trigger | Action |
|---|---|---|---|
| Inactive | Placing | Wave start | Allocate one ambient health + one ambient ammo |
| Placing | Standby | Valid position | Set position, reveal; live timer 0 |
| Standby | Live | First zombie active | Start live timer |
| Placing | Placing | No valid position | Retry after 0.5 s |
| Live | Consumed | Eligible player overlaps | Grant value, free slot |
| Live | Expired | `live_t > live_seconds` | Hide, free slot |
| Consumed/Expired | Waiting | Ambient slot | Start wait timer |
| Waiting | Placing | `wait_t >= wait_seconds` | New position, new generation |
| Consumed/Expired | Inactive | Forced/cache slot | No respawn |
| Live/Placing | Inactive | Wave clear (ambient) | `SUPPLY LOST` cue |
| Live/Placing | Auto-award | Wave clear (forced/cache) | Credit killer, `SALVAGED` cue |
| Any | Inactive | Run reset | Clear timers and positions |

Paused and LevelingUp freeze all timers; no transitions resolve during those
states.

### Interactions with Other Systems

| System | Direction | Interface |
|---|---|---|
| Wave Director | Reads | `wave`, `effective_hp(wave)`, first-zombie-active, wave-clear event |
| Combat | Reads | Kill events, killer, `max_hp`, equipped weapon |
| Player | Reads/Writes | Health, max health, alive; heals and ammo spare |
| Weapons | Reads | `ammo_share`, clip/spare, reserve cap |
| Arena & Cover | Reads | Playable rectangle, cover rectangles, `SetArena()` order |
| Upgrade Draft | Reads | Field Medic and Scavenger stacks |
| HUD & Feedback | Writes | Pickup icons, timer rings, off-screen arcs, collection/waste toasts |
| Local Co-op | Reads | Player count, living players, killer identity; shared tokens, personal salvage |
| Deterministic RNG | Reads | `run_seed`, wave, token index, drop sequence |
| Audio | Writes | Spawn chime, collection cue, expiry fizzle, full/waste thud |


## Formulas

The `salvage_rounds` formula is defined as:

`salvage_rounds = max(1, round_half_up(0.75 × max_hp / 7))`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Enemy spawn max HP | `max_hp` | int | 1–1,280 (waves 1–30) | HP after elite doubling and boss formula; never current HP |
| Salvage rounds | `salvage_rounds` | int | 1–137 (waves 1–30) | Rounds credited to the killer's rifle/SMG |

**Output Range:** 1 to unbounded; 1–137 at waves 1–30. `max(1, …)` prevents zero.
**Example:** wave-30 bloater `max_hp = 75` → `round(0.75 × 75 / 7) = round(8.036) = 8` rounds; wave-30 boss `max_hp = 1,280` → `round(137.14) = 137`.

The `salvage_shells` formula is defined as:

`salvage_shells = max(1, floor(salvage_rounds × 0.20))`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Salvage rounds | `salvage_rounds` | int | 1–137 | Output of the salvage formula |
| Salvage shells | `salvage_shells` | int | 1–27 | Shells credited to the equipped shotgun |

**Output Range:** 1–27 at waves 1–30. A shotgun always receives at least 1 shell per kill.
**Example:** `salvage_rounds = 8` → `floor(8/5) = 1`; `salvage_rounds = 137` → `floor(137/5) = 27`.

The `ammo_pickup_value` formula is defined as:

`ammo_pickup_value = round_half_up((16 + 8 × S) × sh)`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Scavenger stacks | `S` | int | 0–3 | Draft stacks |
| Weapon ammo share | `sh` | float | 0.20 / 0.50 / 1.00 | Shotgun / SMG / rifle |
| Ammo value | `ammo_pickup_value` | int | 3–40 | Rounds or shells granted by an elite token or boss cache |

**Output Range:** 3–40. Minimum is shotgun at S0 (`16 × 0.20 = 3.2 → 3`); maximum is rifle at S3 (`40`).
**Example:** S1 SMG → `24 × 0.5 = 12`; S0 shotgun → `3`; S3 rifle → `40`.

The `health_pickup_value` formula is defined as:

`health_pickup_value = 50 + 25 × FM`

`health_grant = min(health_pickup_value, player_max_hp − player_health)` when `player_health < player_max_hp`; otherwise the token is ineligible and not consumed.

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Field Medic stacks | `FM` | int | 0–3 | Draft stacks |
| Player max health | `player_max_hp` | float | 100–150 | After Vitality |
| Player health | `player_health` | float | 0–`player_max_hp` | Current health |
| Health value | `health_pickup_value` | int | 50–125 | Raw token value |
| Health grant | `health_grant` | float | 0–125 | Actual heal, capped by missing health |

**Output Range:** 50–125 raw; 0–125 applied. Zero only at full health, where the token remains live showing `FULL`.
**Example:** FM2, player 80/100 → value 100, grant `min(100, 20) = 20`.

The `live_seconds` formula is defined as:

`live_seconds = 6 + 0.6 × stacks`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Timer stacks | `stacks` | int | 0–3 | Field Medic for health, Scavenger for ammo |
| Live seconds | `live_seconds` | float | 6.0–7.8 | Time from reveal to expiry |

**Output Range:** 6.0–7.8 s. Use integer tenths (`60 + 6 × stacks`) for comparison.
**Example:** FM3 → 7.8 s; S3 → 7.8 s.

The `wait_seconds` formula is defined as:

`wait_seconds = max(1, 6 − 0.6 × stacks)`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Timer stacks | `stacks` | int | 0–3 | Field Medic for health, Scavenger for ammo |
| Wait seconds | `wait_seconds` | float | 4.2–6.0 | Refill delay after collection or expiry |

**Output Range:** 4.2–6.0 s at 0–3 stacks; clamps at 1.0 s above 8 stacks. Never zero or negative.
**Example:** S3 → 4.2 s; S0 → 6.0 s.

The `wave_round_need` formula is defined as:

`wave_round_need = Σ_i 3 × ceil(hp_i / 21)` for the wave-start roster

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Roster enemy max HP | `hp_i` | int | 1–1,280+ | Spawn HP, elite doubling included |
| Roster count | `N` | int | 5–128 | Wave-start regulars + boss, co-op surcharge included |
| Rounds needed | `wave_round_need` | int | 15–1,695 (waves 1–30) | Rifle-equivalent rounds at 21 damage/shot |

**Output Range:** 15 at wave 1 to 939 baseline / ~1,099 expected / 1,695 ceiling at wave 30. Exclude boss phase adds; they spawn after the ambient value is fixed.
**Example:** wave 30 expected roster → ~1,099 rounds; `round(0.15 × 1,099) = 165`.

The `ambient_ammo_value` formula is defined as:

`B = 16 + 8 × S`
`ambient_ammo_value = wave < 20 ? B : max(B, round_half_up(0.15 × wave_round_need))`
`ambient_ammo_grant = round_half_up(ambient_ammo_value × sh)`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Wave | `wave` | int | 1+ | Current wave; gate applies from 20 |
| Scavenger stacks | `S` | int | 0–3 | Draft stacks |
| Base value | `B` | int | 16–40 | `16 + 8 × S` |
| Rounds needed | `wave_round_need` | int | 15–1,695 | Output of the need formula |
| Raw ambient value | `ambient_ammo_value` | int | 16–231 | Before weapon share |
| Ambient grant | `ambient_ammo_grant` | int | 3–231 | Actual rounds/shells |

**Output Range:** 16 raw before wave 20; 54–165 raw at waves 20–30 (expected); 3–231 after share. Never zero.
**Example:** wave 30 expected, S0 → `max(16, 165) = 165` rifle, 83 SMG, 33 shells.

The `reserve_cap` formula is defined as:

`reserve_cap(weapon) = 240 if weapon ∈ {rifle, SMG}; 40 if weapon = shotgun`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Equipped weapon | `weapon` | enum | rifle / SMG / shotgun | One weapon per run |
| Reserve cap | `reserve_cap` | int | 40 or 240 | Maximum spare rounds or shells |

**Output Range:** exactly 40 or 240. No zero.
**Example:** rifle at 235 spare + 16 pickup → grant 5, spare 240; next token is ineligible (`AMMO FULL`).

The `collection_distance` formula is defined as:

`collection_distance = player_body_radius + token_radius = 20 + 26 = 46`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Player body radius | `player_body_radius` | float | 20 px | Player collision circle |
| Token radius | `token_radius` | float | 26 px | Pickup circle |
| Collection distance | `collection_distance` | float | 46 px | Center-to-center overlap threshold |

**Output Range:** constant 46 px; squared threshold 2,116 px². No scaling with arena or wave.
**Example:** player `(300,300)`, token `(340,320)` → `40² + 20² = 2,000 ≤ 2,116` → collected.


## Edge Cases

- **If no placement candidate passes after 32 attempts and the lattice scan**:
  keep the token invisible, retry every 0.5 s, and never relax cover, 60 px cover
  clearance, 180 px player clearance, or the 300 px minimum. At wave end,
  discard a pending ambient token; auto-award a pending forced token to the
  killer.
- **If a boss or elite dies as the last kill**: convert its uncollected drop
  into an auto-award on the level-up screen (`SALVAGED`) instead of deleting it.
  Award to the killer if alive, otherwise the first living player; discard only
  if no player is alive.
- **If a boss and an elite die in the same frame**: process in kill order. The
  boss cache and elite ammo use separate forced slots; both can exist. Salvage
  is credited per kill.
- **If two living players overlap the same token**: the lowest player index
  collects the whole token; no split or duplicate. A dead player is skipped.
- **If a player at full health touches a health token**: do not consume it; show
  `FULL`, keep the token live, and let a damaged co-op partner collect it.
- **If a player at the ammo cap touches an ammo token**: do not consume it; show
  `AMMO FULL`, keep it live, and let a partner with room collect it.
- **If a token expires on the same frame a player overlaps it**: resolve timers
  first. If `live_t > live_seconds`, expiry wins; if `live_t == live_seconds`,
  collection wins. One transition per token per frame.
- **If the wave clears with ambient tokens live**: despawn them with
  `SUPPLY LOST`; do not carry them into the draft or next wave. Forced ammo and
  boss caches auto-award instead.
- **If Field Medic or Scavenger is acquired mid-run**: active tokens keep their
  snapshotted value and remaining timer; the new stacks apply to the next spawn.
  No retroactive refill.
- **If a boss dies out of bounds or inside cover**: resolve the cache out of
  cover and clamp to the playable rectangle. If no valid point exists,
  auto-award the cache instead of spawning it.
- **If the shotgun's salvage rounds are below 5**: grant 1 shell due to the
  `max(1, …)` floor. This parity with low-HP rifle kills is intentional.
- **If an enemy's spawn max HP is unavailable at death**: treat it as 1 and grant
  1 salvage; never read current HP. Capture max HP at spawn.
- **If a player dies while a forced token is pending**: the token remains shared
  for the surviving player; if no player survives, discard it.
- **If `--wave N` quick-starts at wave 20+**: apply the late-wave floor using
  that wave's roster; do not spawn earlier waves' tokens.
- **If the game pauses or enters the draft**: freeze live/wait timers; no
  collection, expiry, or placement retry resolves during those states.
- **If an elite ammo drop arrives while a forced ammo token is already live**:
  the newest forced token replaces the oldest forced token; the ambient
  wave-start token is never replaced.
- **If the ambient pair cannot maintain 35% separation after one respawns**:
  keep the new token pending until a valid opposite-half-plane position exists;
  do not spawn it on top of the other token.
- **If both players are at the ammo cap and a boss cache is live at wave
  clear**: skip the auto-award because no value can be granted; show
  `AMMO FULL` and discard the cache without a silent loss.


## Dependencies

### Hard dependencies

| System | Direction | Interface | Status |
|---|---|---|---|
| SYS-05 Player | Reads/Writes | Health, max health, body radius 20 px, alive/dead, player index | No GDD — provisional |
| SYS-06 Arena & Cover | Reads | Playable rectangle, cover rectangles, 60 px inset, `SetArena()` ordering | No GDD — provisional |
| SYS-07 Weapons & Ammo | Reads | Equipped weapon, `ammo_share`, clip/spare, reserve cap | No GDD — provisional |
| SYS-09 Combat | Reads | Kill event, killer, spawn max HP, damage/shot (21 reference) | GDD exists |
| SYS-10 Wave Director | Reads | Wave number, wave-start roster HP, first-zombie-active, wave-clear | GDD exists |
| SYS-03 Deterministic RNG | Reads | Run seed, per-token streams | GDD exists |

### Soft dependencies

| System | Direction | Interface | Status |
|---|---|---|---|
| SYS-12 Upgrade Draft | Reads | Field Medic and Scavenger stacks | GDD exists |
| SYS-13 HUD & Feedback | Writes | Pickup beams, timer rings, off-screen arcs, toasts | GDD exists |
| SYS-14 Local Co-op | Reads | Player count, living players, killer identity | GDD exists |
| SYS-15 Audio & Music | Writes | Spawn/collect/expiry/full cues | GDD exists |
| SYS-01 Meta/Economy | Reads | Deep Pockets affects starting spare only, never pickup value | GDD exists |

### Bidirectional checks

- **Combat** lists Pickups as a soft dependency for boss/elite drops and
  heal/ammo restore — compatible. Combat says elites drop "pickups" generically;
  Pickups owns the ammo-only elite drop, so Combat should point here for the
  type.
- **Wave** lists Pickups as writing `SetArena()` at wave start. The direction is
  reversed: Wave calls `SetArena()`; Pickups reads the result. Correct the Wave
  interaction table.
- **Wave** currently says a last-kill boss/elite drop is lost. Pickups replaces
  that with the level-up auto-award; update the Wave edge case.
- **Upgrade** already says Field Medic/Scavenger upgrade the single shared
  pickup — compatible with the per-type timers.
- **Economy** says Deep Pockets affects starting spare only. The build still
  multiplies pickup value by `ammoMetaMultiplier_`; Pickups requires that to be
  removed.
- **Player/Arena/Weapons** have no standalone GDDs. Their contracts above are
  provisional and must be ratified before implementation.


## Tuning Knobs

| Knob | Default | Safe range | If too high | If too low |
|---|---|---|---|---|
| Salvage divisor | 7 | 6–10 | Kills give too little ammo; late waves starve | Kills give too much; pickups become decorative |
| Salvage coefficient | 0.75 | 0.65–0.85 | Reserve caps constantly; tokens expire unused | The 85%-hit wave-30 case starves |
| Shotgun salvage share | 0.20 | 0.15–0.25 | Shotgun never runs dry | Shotgun cannot sustain past wave 20 |
| Scavenger ammo/stack | +8 | +6–+10 | Early/mid ammo trivial | Scavenger feels dead before the late floor |
| Field Medic health/stack | +25 | +20–+30 | FM3 over-heals baseline 100 HP | Health pickups feel negligible |
| Live timer/stack | +0.6 s | +0.4–+0.8 | Tokens stay too long; clutter | Forced drops expire before reach |
| Wait reduction/stack | −0.6 s | −0.4–−0.8 | Refill loop becomes a metronome | Scavenger's throughput identity disappears |
| Late-floor coefficient | 0.15 | 0.10–0.20 | Late ambient token trivializes ammo | Late waves depend on every elite drop |
| Rifle/SMG reserve cap | 240 | 180–300 | Hoarding; pickups still decorative | Clips the 210 max Extended Mag load |
| Shotgun reserve cap | 40 | 30–60 | Shotgun never fears ammo | Shotgun cannot use its burst role |
| Ambient band minimum | 300 px | 250–400 | Free top-up; no crossing decision | Too many unreachable drops |
| Ambient band maximum | 700 px | 500–900 | Hidden speed check on 650-side arenas | Drops cluster near the player |
| Pair separation | 35% arena | 25–50% | One side of the arena is starved | Both pickups collected in one trip |
| Cover clearance | 60 px | 50–80 | Few valid positions; constant retries | Drops tuck beside cover; safe grabs |
| Player clearance | 180 px | 150–220 | Fewer valid positions | Spawn on top of the player |
| Token edge spacing | 40 px | 30–60 | Visual overlap | Tokens stack into one silhouette |
| Placement attempts | 32 | 16–64 | Minor CPU cost only | More lattice fallbacks |
| Retry interval | 0.5 s | 0.25–1.0 | Long invisible gaps | Placement churn |

**Interactions**

- Salvage coefficient, late floor, and reserve cap are coupled. Raising salvage
  or lowering the cap makes the ambient/elite tokens decorative; lowering
  salvage or raising the cap makes them mandatory.
- At waves 20–30 the late floor (54–165 raw) exceeds Scavenger's base (16–40),
  so Scavenger no longer changes the ambient token's amount. It still changes
  elite/boss-cache value and reload speed. If late Scavenger ammo identity is
  required, make the floor additive (`B + floor`) or lower the coefficient.
- The 240 cap is set above the 210 maximum Extended Mag starting load (70 clip +
  140 spare). A cap below 210 would nerf that meta upgrade.
- FM3 value 125 exceeds the baseline 100 max health; the excess is intentionally
  capped. This is over-heal, not a formula error.
- Do not tune `collection_distance` independently of the 20 px player and 26 px
  token radii; it is derived (46 px).


## Visual/Audio Requirements

- **Palette:** health mint `{96,244,178}`, ammo amber `{255,196,72}`,
  cache white `{242,246,250}`, muted `{168,176,184}`. Forbidden in pickup
  feedback: damage red, boss red, elite gold.
- **World token:** existing `health_pickup.png` / `ammo_pickup.png`, plus a
  ground marker (mint circle + ticks for health, amber hex for ammo, white
  diamond for cache). No expanding ring — the enemy spawn ring is a threat cue.
- **Reveal:** thin vertical supply beam, 190 px tall, 12 px wide, gradient fade,
  0.2 s in. Health reveals first, ammo 120 ms later at wave start; refill
  reveals only when on-screen or within 900 px.
- **Live timer:** draining ring around the token, clockwise from 12 o'clock; no
  number. Last 1.5 s: hot-white, broken segments, 2 Hz pulse. Last 0.5 s: tick
  cue.
- **Collection:** 0.30 s inward burst + white flash + texture ghost; health and
  ammo use different colors and pitch.
- **Expiry:** 0.45 s contracting ring + falling muted particles; `SUPPLY LOST`
  on the draft screen for ambient tokens.
- **Boss cache:** composite token with split mint/amber ring and `CACHE` label;
  one collision grants both.
- **Off-screen:** edge icon + timer arc for at most two pickups, prioritized by
  need then proximity; distinct from enemy edge wedges.
- **Toasts:** `+16 RIFLE`, `+8 SMG`, `+3 SHOTGUN`, `+10 HP`, `FULL`,
  `AMMO FULL`, `SUPPLY LOST`, `SALVAGED`, and
  `BOSS CACHE: +50 HP, +16 AMMO`.
- **Audio:** reuse `pickup.wav` at pitch 0.85 for health and 1.20 for ammo
  (replaces the current `reload.wav` misuse). Add four short tonal cues: supply
  reveal, elite ammo drop, boss cache, expiry/fizzle, plus a muted full/waste
  tap. No impact, sub-bass, or mechanical clack.
- **Accessibility:** shape + text + sound, not color alone; blink capped at
  2 Hz; no screen shake or red vignette for pickup events.
- **Performance:** primitives + the two existing textures; no shaders, no
  raygui, no per-frame allocations; world VFX in world pixels, HUD in screen
  space (`s = height / 720`).


## UI Requirements

- **Pickup toasts:** reuse the existing damage-number text path; max 4 pickup
  toasts, 1.2 s life, screen-space. Show the actual amount and weapon:
  `+16 RIFLE`, `+8 SMG`, `+3 SHOTGUN`, `+10 HP`, `FULL`, `AMMO FULL`.
- **HUD pulses:** health bar pulses mint only when health increases; ammo
  counter pulses amber only when spare increases. No pulse on `FULL` or
  `AMMO FULL`.
- **Off-screen:** at most two edge icons with a timer arc, prioritized by need
  (health below 50%, ammo below 25%) then proximity. Distinct icon shape from
  enemy wedges.
- **Draft screen:** show `SUPPLY LOST` for expired ambient tokens and
  `SALVAGED` for auto-awarded forced/boss drops, with the actual amounts.
- **Boss cache:** one combined toast `BOSS CACHE: +50 HP, +16 AMMO`; one HUD
  pulse for each resource.
- **Full/waste:** pre-collection `FULL` / `AMMO FULL` badge on the token; no
  consume, no pulse. A muted tap on contact.
- **Co-op:** shared token; toast identifies the collector (`P1 +16 RIFLE`).
  Salvage is personal, so show `+8 SALVAGE` only to the killer; no global toast.
- **Scaling:** new UI uses `s = GetScreenHeight() / 720`; world-space elements
  stay in world pixels. No new menu, no raygui.
- **Performance:** reuse the existing font/text helpers; no per-frame allocation
  in the toast/indicator path.


## Acceptance Criteria

1. **Ambient pair and activation.**
   **GIVEN** a wave start with cover and 1–2 living players placed, **WHEN** the
   pair resolves, **THEN** exactly one health and one ammo ambient token exist,
   each with center-to-center distance 300–700 px from its nearest living
   player, Euclidean separation at least 35% of the playable arena side,
   opposite sides of the reference player's vertical centerline, 60 px cover
   clearance (86 px center-to-cover), a cover-free corridor, 92 px center
   spacing, and inside the playable rectangle. At the bell both are visible but
   cannot be collected and their live/wait timers do not decrement. When the
   first zombie's 0.6 s spawn ring expires, both become collectible and timers
   start at `60 + 6×FM` tenths (health) and `60 + 6×S` tenths (ammo).

2. **Placement fallback.**
   **GIVEN** a fixture with no point satisfying criterion 1, **WHEN** placement
   runs, **THEN** no token is rendered or collectible, retries occur every 0.5 s
   (±1 frame), and no candidate is accepted inside cover, within 60 px of cover,
   within 300 px of the nearest living player, within 180 px of any living
   player, or at a logged rejected point. **WHEN** the fixture exposes one valid
   point at t = 2.0 s, **THEN** the token spawns there on the next retry and is
   live. If only the 700 px upper bound fails, the token may spawn beyond 700 px
   after relaxing that bound; if only 35% separation fails, it may spawn closer
   after relaxing separation; opposite-half-plane is relaxed last. The 300 px,
   60 px, 180 px, and 92 px constraints are never relaxed.

3. **Salvage.**
   **GIVEN** a wave-30 bloater with spawn max HP 75 at 1 current HP and a rifle,
   **WHEN** it dies, **THEN** rifle spare increases by 8 and no token is created;
   with an SMG it increases by 8; with a shotgun by 1 shell. **GIVEN** a wave-30
   boss with spawn max HP 1,280, **THEN** rifle/SMG salvage is 137 and shotgun
   salvage is 27. **GIVEN** an elite with spawn max HP 150, **THEN** rifle/SMG
   salvage is 16 and shotgun salvage is 3. **GIVEN** a captured spawn max HP of
   1, **THEN** salvage is 1 round / 1 shell. Current HP is never used.

4. **Elite ammo.**
   **GIVEN** an elite dies at point P, **WHEN** the kill resolves, **THEN**
   exactly one elite-ammo spawn event occurs with raw value
   `round((16 + 8×S)×share)`; at S0 the raw value is 16 (rifle 16, SMG 8,
   shotgun 3), at S1 raw 24 (SMG 12), at S3 raw 40 (rifle 40). A live forced
   token appears at P if P is inside the arena and 60 px clear of cover;
   otherwise it relocates to the nearest valid point, or stays pending. A second
   elite replaces the first forced token; the ambient token is unchanged. Elite
   ammo is exempt from the 180 px player clearance.

5. **Boss cache.**
   **GIVEN** a wave-30 boss with spawn max HP 1,280 dies at P, FM=2, S=1, and a
   rifle killer, **WHEN** the cache is collected by a player at 80/100 health
   and 100 spare, **THEN** health becomes 100 (grant 20) and spare increases by
   24; the cache is consumed once and shows `BOSS CACHE: +50 HP, +16 AMMO`. If
   health is full but ammo has room, grant ammo, consume, and show `FULL`; if
   ammo is capped but health is missing, grant health, consume, and show
   `AMMO FULL`; if both are ineligible, do not consume. If P is invalid,
   relocate to the nearest valid point; if none, auto-award. If the boss is the
   last kill, auto-award at level-up.

6. **Collection.**
   **GIVEN** a living player at `(300,300)` and a live ammo token at
   `(340,320)`, **WHEN** eligible, **THEN** `40² + 20² = 2,000 ≤ 2,116` and the
   token is collected once. **GIVEN** distance² 2,117, **THEN** no collection.
   **GIVEN** two living players overlap the same token in one frame, **THEN**
   only the lowest-index living player collects; the other's ammo is unchanged;
   a dead lower-index player is skipped. **GIVEN** full health, **THEN** no
   consume, `FULL`, token remains live and a damaged partner can collect.
   **GIVEN** ammo at cap, **THEN** no consume, `AMMO FULL`, token remains live
   and a partner with room can collect.

7. **Timers and respawn.**
   **GIVEN** a live health token with FM=2, **WHEN** active, **THEN** live timer
   is 72 tenths (7.2 s); FM0 is 60, FM3 is 78. Ammo uses S. Expiry occurs on the
   first frame with `live_t > live_seconds`; at equality the token is live and
   collection wins. **GIVEN** an ambient token is collected or expires with
   FM=2, **THEN** wait is 48 tenths (4.8 s), then a new token spawns with current
   stacks and a fresh timer. Forced tokens and caches never respawn. **GIVEN**
   pause or level-up for 1.0 s, **THEN** all timers and retries are unchanged.

8. **Late-wave floor.**
   **GIVEN** wave 19, S=0, rifle, **WHEN** the ambient ammo token is created,
   **THEN** raw value 16 (S3: 40). **GIVEN** wave 20+, **THEN** raw is
   `max(16 + 8S, round_half_up(0.15 × wave_round_need))`, where
   `wave_round_need = Σ 3×ceil(hp_i/21)` over the wave-start roster including
   elites and boss and excluding boss phase adds. **GIVEN** a fixture wave-30
   roster with `wave_round_need = 1,099`, S=0, **THEN** raw 165, rifle 165,
   SMG 83, shotgun 33, and exactly one ambient ammo token. Boss phase adds do
   not change the value. `--wave 30` uses the wave-30 roster.

9. **Reserve cap.**
   **GIVEN** rifle at 235 spare, S=0, **WHEN** a 16-round token is collected,
   **THEN** spare becomes 240 (grant 5), 11 are lost, the token is consumed, and
   `AMMO FULL` is shown. **GIVEN** rifle at 240, **WHEN** a 16 token is touched,
   **THEN** no consume, it remains live with `AMMO FULL`, spare stays 240, and a
   partner with room can collect. **GIVEN** a kill grants 8 at 239 spare,
   **THEN** spare becomes 240, 7 are lost, `AMMO FULL` is shown, no token is
   created. **GIVEN** shotgun at 39 shells with a 3-shell token, **THEN** spare
   becomes 40 (grant 1, 2 lost). The SMG cap is 240. Health has no reserve cap;
   Deep Pockets never changes pickup or salvage value.

10. **Wave clear.**
    **GIVEN** wave clear with live ambient tokens, **WHEN** level-up opens,
    **THEN** each is removed, `SUPPLY LOST` shows its uncollected amount, and
    neither carries over; a pending ambient slot is cleared silently. **GIVEN**
    a live or pending forced ammo token or boss cache, **THEN** it auto-awards
    exactly once to the killer if alive, otherwise the first living player, with
    `SALVAGED` and the actual amount; if no player is alive, discard. **GIVEN**
    a boss/elite is the last kill, **THEN** the same auto-award happens at
    level-up. **GIVEN** both players at the ammo cap and a cache live, **THEN**
    no credit, `AMMO FULL`, and discard without a silent loss.

11. **Determinism.**
    **GIVEN** the same effective run seed, wave, arena/cover, player count and
    positions, and FM/S stacks, **WHEN** wave start resolves twice with different
    frame-dt sequences and wall-clock times, **THEN** both ambient tokens have
    identical kind, raw value, and position. **GIVEN** the same kill order and
    drop sequence, **THEN** forced tokens/caches have identical kind, raw value,
    and position. A different seed may change positions; kind/value follow that
    seed's wave plan. Fixed order is health → ammo → players → kills. No
    wall-clock or time-based RNG affects values or positions.

12. **Ammo solvency (balance model).**
    **GIVEN** wave 30 solo, reference rifle (21 damage/shot, 3 rounds/shot,
    S=0, no meta, starting spare 0), a fixture roster with
    `wave_round_need = 1,099`, an 85% collection policy (collect 17 of every 20
    ambient/forced/cache spawns), salvage on every kill, and cap 240 with
    overflow lost, **WHEN** the wave is simulated, **THEN** gross spare credited
    after overflow is at least 1,099; report the exact total. At 100% collection
    it is also at least 1,099; at 0% it equals salvage only and is reported.
    This is a balance-model check, not the wave-clear-time gate.

13. **Types.**
    **GIVEN** all spawn paths over a scripted 30-wave run, **WHEN** every pickup
    entity is enumerated, **THEN** each kind is `health`, `ammo`, or `boss_cache`;
    no score, power-up, weapon, currency, or other token exists. The boss cache
    is one composite token granting both on one collision. The only non-token
    ammo grants are salvage and `SALVAGED` auto-award.

14. **Co-op ownership.**
    **GIVEN** P1 (index 0) and P2 (index 1) both alive and overlapping the same
    ammo token, **WHEN** collection resolves, **THEN** P1 receives the full value
    once and P2's ammo is unchanged. **GIVEN** P2 kills a 75-HP bloater with a
    rifle, **THEN** P2 receives +8 salvage and no token is created. **GIVEN** P2
    kills an elite with 150 max HP and P1 collects the forced ammo, **THEN** P2
    receives +16 salvage personally, P1 receives the token value, the salvage
    toast is P2-only, and the token toast says `P1 +16 RIFLE`. A dead player is
    skipped; auto-award goes to the killer if alive, else the first living
    player.

15. **Feedback (visual/feel gate).**
    **GIVEN** 1280×720 and a live ammo token, **WHEN** observed, **THEN** a
    draining ring is drawn clockwise from 12 o'clock with no number; at ≤1.5 s
    it is hot-white with broken segments at 2 Hz; at ≤0.5 s a tick plays.
    Off-screen shows at most two edge icons with timer arcs, prioritized by need
    (health <50%, ammo <25%) then proximity, with a shape distinct from enemy
    wedges. Collection shows the actual amount and weapon (`+16 RIFLE`,
    `+8 SMG`, `+3 SHOTGUN`, `+10 HP`) for 1.2 s, max 4 toasts; co-op prefixes the
    collector. `FULL`/`AMMO FULL`, `SUPPLY LOST`, and `SALVAGED` appear on the
    token/draft screen; the cache shows `BOSS CACHE: +50 HP, +16 AMMO`. No
    pickup feedback uses damage red, boss red, or elite gold. Screenshot +
    technical-artist sign-off are required.

16. **Performance.**
    **GIVEN** wave 30 co-op, 112 live zombies, four live tokens (ambient health,
    ambient ammo, elite ammo, boss cache), and four active toasts, **WHEN** a
    deterministic benchmark runs 600 frames at 60 Hz after a 60-frame warm-up at
    1280×720, **THEN** p99 frame time ≤16.6 ms and p100 ≤20 ms; the pickup/toast
    path performs zero heap allocations after warm-up. Report p50/p99/p100 and
    allocation count. The same scene without pickups must show a pickup/toast
    delta ≤1.0 ms p99. A dedicated benchmark flag is required; `--profile` is a
    save-profile path and must not be reused.


## Open Questions

1. **The 10–40 s wave gate is still unsigned.** SYS-11 closes the ammo gap, but
   the median rifle is ~110 sustained DPS after reload; wave 30 takes ~64 s even
   with infinite ammo. The gate needs a reload/clip fix (Extended Mag/Kill
   Reload in the reference build) or a late-wave HP reduction. Owner: Wave
   Director + Weapons; resolve before architecture.
2. **Player, Arena, and Weapons have no GDDs.** Their contracts here are
   provisional. Ratify body radius, 60 px inset, ammo shares, and reserve cap
   before implementation.
3. **Scavenger's late identity.** The wave-20+ floor exceeds the Scavenger base,
   so Scavenger no longer changes the ambient token's amount. Keep it as a
   reload/elite-cache upgrade, or make the floor additive (`B + floor`).
4. **Elite drop type.** This GDD makes elites ammo-only. Combat currently says
   "bosses and elites drop pickups" generically; update Combat to point here.
5. **Last-kill drops.** Wave currently documents that a last-kill boss/elite drop
   is lost. This GDD auto-awards it at level-up; update the Wave edge case.
6. **Co-op feel.** Shared first-collector was the user's choice; the
   game-designer recommended both living players receive the effect. Revisit
   after a co-op playtest if the partner feels robbed.
7. **Reserve cap.** 240 rifle/SMG and 40 shells. If SYS-07 rejects a cap, the
   overflow/`AMMO FULL` rules and the ammo-solvency margin change.
8. **Asset work.** Four new tonal cues (reveal, elite drop, boss cache,
   expiry/fizzle) plus a full/waste tap are specified but not authored.
9. **Implementation deltas.** The build currently adds ammo to every unlocked
   weapon, applies Deep Pockets to pickup value, uses 0.35/1.0 shares, a 50 px
   inset, one token per type, and no player/spacing checks. All must change.
10. **Determinism scope.** Wave-start amount depends on elite rolls; it is
    reproducible per seed but not identical across runs. If cross-run identical
    amounts are required, use the baseline roster or a fixed elite plan.
11. **Boss phase adds** are excluded from `wave_round_need` because the ambient
    value is fixed at wave start. State this in implementation tests.
12. **SMG/shotgun validation.** The arithmetic model passes (min ratios 1.22 /
    1.55), but no playtest has validated the 0.5/0.20 shares with the new
    salvage and cap.
13. **ADR required.** New gameplay system + cross-cutting supply changes need an
    ADR before implementation, per project rules. Candidate decision: auto-credit
    salvage + capped reserve + shared token model.

