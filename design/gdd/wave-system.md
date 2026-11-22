# Wave Director

> **Status**: In Review — major revision 2026-11-22; implementation deltas pending
> **Author**: game-designer / systems-designer (documented from code)
> **Last Updated**: 2026-11-22
> **Last Verified**: source inspection 2026-11-22 (suites not executed)
> **Implements Pillar**: Escalating, readable pressure

> **Source of truth**: `src/Waves.{h,cpp}`, `src/Game.cpp` (`BeginWave`,
> `EnterLevelingUp`, `EnterGameOver`), `src/Arena.cpp`, `src/Zombie.h`.
> ADR-0001 covers cover generation; the combat GDD covers damage and rewards.
> This GDD is the **revised contract**; implementation deltas are listed at the
> end.

## Overview

The Wave Director decides how many zombies spawn, what types they are, how fast
and tough they become, when a boss appears, when an elite is guaranteed, and how
the arena grows. It owns the wave lifecycle: spawn → fight → clear → draft →
next wave, death → game over, or victory. It also owns the threat budget that
keeps the pressure curve honest.

## Player Fantasy

The run is a **three-act arc**, not a monotone ramp:

1. **Desperation (waves 1–8):** pressure outruns power. The player scrambles,
   learns enemy tells, and survives on positioning and ammo discipline.
2. **Tension (waves 9–20):** power and pressure are roughly matched. Build
   choices start to pay off, but a mistake still ends the run.
3. **Power trip (waves 21–30):** the build outruns the horde. The player cuts
   through it, but threat variety, elites, and bosses keep rising so the
   spectacle and the risk never disappear.

Every threat is telegraphed by silhouette, tint, sound, and a visible spawn.
The player should always be able to name what hit them and why.

## Detailed Design

### Core Rules

1. **Wave count.** A normal solo wave spawns `5 + 2 × (wave − 1)` zombies.
   In co-op, add `3 + floor(1.5 × wave)`. Boss waves **add** one boss instead
   of trading regulars: `count = count + 1`. Waves 1–4 therefore spawn 5, 7, 9, 11 solo;
   wave 5 is a boss wave with 14 (13 regulars + 1 boss).
2. **Boss cadence.** Every fifth wave is a boss wave. One boss is added to the
   normal count. The boss is placed first in the composition and is the only
   boss.
3. **Composition unlocks.** Bloaters, chasers, and crawlers are always
   available. Spitters unlock at wave 2, sprinters at wave 3, exploders at
   wave 4. From its unlock wave, a new type is capped at 40% of the regular
   slots so a single roll cannot define the wave.
4. **Weighted picks.** Each regular slot rolls independently against the current
   weights. Weights are not normalized; the total is recomputed per pick.
5. **Spawn placement and activation.** Zombies spawn on a random arena edge,
   60 px inside the playable boundary; the **body circle**, not just the point,
   must be cover-free. Spawns are spaced at least 40 px apart and at least
   180 px from a living player. A spawn is **inactive for 0.6 s** (the ring
   pulses) and cannot move, damage, or be damaged until the ring expires.
   Off-screen spawns show a short edge indicator. Cover is inset **150 px**
   (three tiles), so the 110 px spawn band is outside the cover band; the
   body-circle check handles large bosses.
6. **Per-zombie variance and health.** Speed is multiplied by a random factor in
   `[0.91, 1.0)` and then receives the per-wave speed bonus. Regular zombies
   gain health by `base_health + floor((wave − 1)² / 12)`. Boss health is
   `40 + 18 × wave + 10 × floor((wave − 1)² / 12)`. Listed health is effective
   health; the combat GDD owns the `health <= 0` death rule.
7. **Elite assignment.** From wave 4 onward, the first non-boss zombie is always
   elite; every other non-boss zombie rolls
   `min(0.05 + 0.015 × wave, 0.25)`. Bosses are never elite.
8. **Arena growth and camera.** Arena side is
   `min(650 × (1 + 0.1 × wave), 1800)`. Cover count is
   `min(2 + floor(wave / 3), 12)`. The camera target is clamped to the arena,
   and off-screen threats show edge indicators. The playable rectangle is inset
   50 px from the drawn wall; every mover is clamped to it.
9. **Wave start.** Both players spawn at the arena centre (offset ±40 px in
   co-op), dead players revive at 50% health, pickups spawn, the banner shows
   "WAVE N" for 2.4 s **inside the HUD band**, and the gameplay music starts.
10. **Wave clear.** When the last zombie dies and all active exploder fuses have
    resolved, award `5 + 2 × wave` base scrap (the economy applies the bank
    multiplier), open the upgrade draft, and switch to level-up music. The next
    wave begins only after the player picks a card.
11. **Game over.** When every player is dead, play the death sound, stop music,
    save the hi-score, and finish the run (persisting scrap and stats).
12. **Victory and terminus.** Clearing wave 30 awards `victory_bonus = 5,000`
    score, opens the wave-30 draft, and then shows the Victory screen **once**.
    The player may continue into endless waves. From wave 31, regular HP is
    frozen at the wave-30 value, count is capped by the live cap, elite chance
    stays 25%, and boss HP for `wave > 30` is `boss_health(30) + 30 × (wave −
    30)`. The Bunker remains capped;
    after every row is maxed, banked scrap is lifetime/scoreboard only. A run
    otherwise ends only on death or quit.
13. **Threat budget.** Each wave has a target clear time
    `target_clear_seconds = clamp(10 + wave, 10, 40)` against an expected
    player DPS of `min(200, 5 × 1.18^(wave − 1))`. The reference build is a
    wave-indexed median rifle: 4 Hollow Point, 4 Rapid Fire by wave 20 (5 by
    wave 30), 2 Weapon Tuning, and 1 Bullet Storm by wave 20 (2 by wave 30),
    with the remaining drafts on build/utility. The max SMG+Storm build (~418
    DPS) is the power-trip ceiling, not the budget. Measured clear time 10–40 s
    is the gate for waves 5–30; wave 1 is the onboarding exception.

### Enemy AI Contract (provisional; owned by SYS-08)

- Every zombie targets the **nearest living player** each frame; ties go to the
  lower player index. No aggro memory, hearing, or hysteresis.
- Non-ranged zombies chase directly. When blocked by cover for more than 0.5 s,
  they steer along the face toward the nearer cover edge (axis sliding alone is
  not sufficient). They never stall permanently on a flat face.
- Melee movers are clamped to the playable rectangle after movement.
- Spitters: advance while distance > 500, retreat while distance < 350, hold in
  the 350–500 band. Retreat is clamped to the playable rectangle. They fire only
  with line of sight to the target (cover blocks the shot); first shot 1.2 s
  after entering range, then every 2.2 s. They lead by the projectile's flight
  time (distance / 280 s), not a fixed 0.3 s.
- The boss uses the melee chase plus a gap-closer: every 4 s, if the nearest
  player is more than 120 px away, it charges at 320 px/s for 0.8 s, leading the
  target by 0.2 s. At 60% and 30% health it becomes invulnerable until both
  phase adds die or 2.0 s elapse, whichever is longer; it summons 2 adds with
  the standard spawn ring and emits a shockwave, with an immune ring/floating
  cue and boss-bar gate markers. After the second phase it gains +20% base
  chase speed (charge stays 320). Phase gates resolve sequentially; excess
  damage is capped at the gate threshold.
- Zombies do not collide with each other; a light separation radius (24 px) is
  a required invariant for SYS-08 so the horde presents a front, not a point.

### Enemy Stat Reference

These are the revised base values the Wave Director writes into each zombie.
They will move to an Enemy Roster GDD when it is authored; until then this table
is the cross-document source of truth. All speeds are true px/s in every
direction (movement vectors are normalized).

| Type | Base HP | Base speed | Wave speed bonus | Scale | Contact damage | Score | Scrap | Behaviour |
|---|---|---|---|---|---|---|---|---|
| Bloater | 5 | 40 | 50 | 1.00 | 10 | 10 | 1 | Slow, high health |
| Chaser | 1 | 70 | 70 | 1.00 | 10 | 10 | 1 | Direct chase |
| Crawler | 3 | 20 | 30 | 1.00 | 10 | 10 | 1 | Very slow |
| Spitter | 2 | 30 | 15 | 0.95 | 10 | 15 | 2 | Ranged; 350–500 band; LoS only; 600 px range; leads by flight time |
| Sprinter | 1 | 260 | 30 | 0.85 | 10 | 15 | 2 | Fast, fragile; catches a retreating player |
| Exploder | 3 | 55 | 15 | 1.10 | 15 | 15 | 2 | 0.35 s fuse; 20 damage / 110 px blast |
| Boss | 40 + 18×wave + 10×floor((wave−1)²/12) | 90 | 30 | 2.20 | 20 | 100 + 50×wave | 25 + 5×wave | Every 5th wave; health bar; charge; 2 phase gates |
| Elite modifier | ×2 | ×1.15 | — | ×1.25 | ×1.5 | ×3 | ×3 | Never applied to a boss; guaranteed pickup drop |

**Health scaling:** regular zombies add `floor((wave − 1)² / 12)` health; the
boss uses `40 + 18 × wave + 10 × floor((wave − 1)² / 12)`, which includes a
regular-scaling term, plus two phase gates. Because the death rule
is `health <= 0`, listed HP is also the number of 1-damage hits needed.

### States and Transitions

| From | To | Trigger |
|---|---|---|
| Title | Loadout | Start a new run |
| Loadout | Playing | Weapon chosen; wave 1 begins |
| Title | Playing | `--wave N` quick start (skips loadout/draft) |
| LevelingUp | Playing | Player picks an upgrade card |
| Playing | LevelingUp | Last zombie and active fuses resolve |
| Playing | LevelingUp | Wave 30 cleared (draft first) |
| LevelingUp | Victory | Card picked on wave 30 |
| Victory | Playing | Continue into endless |
| Victory | GameOver | End the run |
| Playing | Paused | Pause input |
| Paused | Playing | Pause input |
| Playing | GameOver | All players dead |
| GameOver | Loadout | New run (Bunker optional) |

### Interactions with Other Systems

| System | Direction | Interface |
|---|---|---|
| Deterministic RNG | Reads | One spawn RNG per wave, seeded from run seed |
| Enemy Roster & AI | Writes | Composition types, spawn positions, speed/health, elite flag |
| Arena & Cover | Writes | `SetWave(wave, seed)` rebuilds size and cover; playable rectangle |
| Combat Resolution | Reads | Reads `zombiesAlive_` and active fuses; owns `EnterLevelingUp()` / `EnterGameOver()` |
| Upgrade Draft | Writes | `GenerateDraft()` on wave clear |
| Pickups | Writes | `SetArena()` at wave start |
| Meta Progression | Writes | Wave reached, best wave, wave-clear scrap |
| HUD & Feedback | Writes | Wave banner, boss bar, wave/zombie counters, edge indicators |
| Audio & Music | Writes | Gameplay music on start, level-up music on clear |
| Local Co-op | Reads | Player count scales the horde; revive at wave start |
| Save & Persistence | Writes | Best wave and victory flag |

## Formulas

### Wave zombie count

`count = 5 + 2 × (max(1, wave) − 1) + (coop ? 3 + floor(1.5 × max(1, wave)) : 0)`
`if wave >= 5 and wave % 5 == 0: count = count + 1`

**Variables:** wave 1+, coop bool. **Output Range:** 5 at wave 1 to 64 at
wave 30 solo and 112 co-op; live entities are capped at 96/128 (see
Performance). **Example:** wave 5 solo = `5 + 8 = 13` → boss wave `13 + 1 =
14`. Wave 5 co-op = `5 + 8 + 3 + floor(7.5) = 23` → `24`.

### Threat budget

`expected_dps(wave) = min(200, 5 × 1.18 ^ (wave − 1))`
`target_clear_seconds(wave) = clamp(10 + wave, 10, 40)`
`clear_time(wave) = effective_hp(wave) / expected_dps(wave)`

| Variable | Symbol | Type | Range | Description |
|---|---|---|---|---|
| Wave | `wave` | int | 1+ | Current wave |
| Expected DPS | `expected_dps` | float | 5–200 | Median accumulated build, capped at 200 |
| Target clear | `target_clear_seconds` | float | 10–40 | Intended act pacing |
| Effective HP | `effective_hp` | float | 14+ | Base HP including elites |

| Wave | Count solo | Base HP | Effective HP (approx) | Expected DPS | Target s | Clear s |
|---|---|---|---|---|---|---|
| 1 | 5 | 14 | 14 | 5 | 11 | 2.8 |
| 5 | 14 | 190 | 228 | 9.7 | 15 | 23.5 |
| 10 | 24 | 483 | 580 | 22.3 | 20 | 26.0 |
| 20 | 44 | 2,111 | 2,533 | 116 | 30 | 21.8 |
| 30 | 64 | 5,867 | 7,040 | 200 | 40 | 35.2 |

Effective HP includes the guaranteed elite and random elites (~+20%). Boss
rows add the two 2.0 s phase gates and their adds (wave 30 ≈ +5 s, to ~40.7 s).
The target column encodes the intended three-act arc; the gate is **measured
clear time 10–40 s for waves 5–30**, not the per-wave target. Wave 1 is the
onboarding exception. Waves 5/10 sit at the high end; waves 20/30 sit lower.

**Ammo feasibility is an open dependency.** The table uses burst DPS. Reload
and the current 12 s pickup cycle give the median rifle ~100 sustained DPS,
and wave-30 ammo is short. The 10–40 s gate is not signed off until SYS-11
Pickups defines the ammo faucet (per-kill drops or wave resupply) or late-wave
HP is reduced; the table is a lower bound, not a measured pass. Co-op uses the `3 + floor(1.5w)` surcharge and the 128 live
cap; validate the same clear-time band separately.

### Composition weights

The composition_weights table is defined as:

| Type | Weight | Unlock |
|------|--------|--------|
| Bloater | 35 | Wave 1 |
| Chaser | 45 | Wave 1 |
| Crawler | 30 | Wave 1 |
| Spitter | 15 | Wave 2 |
| Sprinter | 12 | Wave 3 |
| Exploder | 10 | Wave 4 |
| Boss | 0 (explicit) | Every 5th wave |

Each regular pick rolls `[0, totalWeight)` and walks the cumulative weights. A
newly unlocked type is capped at 40% of the regular slots.
**Output Range:** any unlocked type can appear; the boss never enters the pool.
**Example:** wave 3 weights total `35 + 45 + 30 + 15 + 12 = 137`; chaser covers
rolls 35–79, crawler 80–109, spitter 110–124, sprinter 125–136.

### Zombie speed

`speed = (BaseSpeed(type) × variance + WaveSpeedBonus(type) × 0.1 × wave) × (elite ? 1.15 : 1)`

**Variables:** base 20–260; variance 0.91–1.0; bonus 10–70.
**Output Range:** ~21.2 px/s (crawler, wave 1, minimum variance) to **437
px/s** (elite sprinter, wave 40). **Example:** sprinter wave 10 =
`260 × 0.95 + 30 × 1.0 = 277`; elite sprinter wave 40 =
`(260 + 120) × 1.15 = 437`; elite chaser wave 40 = `(70 + 280) × 1.15 = 402.5`.

### Regular enemy health

`health = base_health(type) + floor((wave − 1)² / 12)` for `wave ≥ 1`

**Variables:** base 1–5; wave 1+. **Output Range:** 1 at wave 1 to 75 at
wave 30 for a bloater. **Example:** chaser: 1 HP at waves 1–4, 2 at wave 5,
6 at wave 9. Bloater: 10 HP at wave 9. Elites double the result.

### Boss health

`boss_health = 40 + 18 × wave + 10 × floor((wave − 1)² / 12)`

**Variables:** wave 5, 10, 15… **Output Range:** 140 at wave 5, 280 at wave 10,
700 at wave 20, 1,280 at wave 30; the quadratic term is frozen at wave 30, and
boss HP grows +30/wave thereafter. **Example:** wave 15 boss = `40 + 270 + 160 = 470`.
Each boss has two phase gates (60%/30% HP, 2.0 s invulnerable, 2 adds); target
TTK is 10–20 s for the expected build.

### Elite chance

`elite_chance = min(0.05 + 0.015 × wave, 0.25)` for non-boss zombies when
`wave ≥ 4`

**Output Range:** 11% at wave 4 to a 25% cap from wave 14.
**Example:** wave 8 = `0.05 + 0.12 = 0.17`.

### Arena size and cover count

`arena_side = min(650 × (1 + 0.1 × wave), 1800)`
`cover_count = min(2 + floor(max(1, wave) / 3), 12)` for `wave ≥ 1`.

**Output Range:** 715 px / 2 cover at wave 1 → 1300 px / 5 at wave 10 →
1800 px / 12 from wave 30. **Example:** wave 1 = `650 × 1.1 = 715`.

### Wave-clear scrap

`wave_clear_scrap_base = 5 + 2 × wave`

The economy applies the Scrap Magnet **bank multiplier once at `FinishRun`**;
see `economy-system.md`. **Output Range:** 7 at wave 1, 25 at wave 10, 65 at
wave 30. **Example:** clearing wave 5 awards 15 base scrap; with Magnet 5 and
5 waves cleared the bank receives `round(15 × 2.0 × 1.15) = 35` at run end.

## Performance & Scaling

Budget: **16.6 ms/frame at 60 fps, 1280×720**. Sub-budgets: simulation 2 ms,
entity draw 2 ms, floor draw 2 ms, HUD/text 1 ms, feedback 1 ms.

- **Live caps:** 96 zombies solo / 128 co-op. Excess spawns wait in a queue
  released at 6–8/s. Boss and elite spawns are never queued.
- **Arena cap:** 1,800 px, so floor tiles are bounded at 36² = 1,296.
- **Floor culling:** only tiles intersecting the camera rect (+1 tile margin)
  are submitted, or the static floor is baked into a `RenderTexture2D` per wave.
- **Feedback caps:** 160 bullets, 120 blood decals, 64 damage numbers,
  64 shockwaves, 64 enemy shots; all oldest-out.
- **Camera:** target clamped to the arena; off-screen threats show edge pips.
- **Acceptance:** a `--wave N --profile` smoke pass reports min/avg/p99 frame
  time for 600 frames at waves 1/10/20/30, solo and co-op; average ≤ 16.6 ms and
  p99 ≤ 20 ms; floor submissions ≤ 2× visible tiles.

## Edge Cases

- **If `--wave N` quick-starts a run**: skip the initial loadout/draft, spawn the
  wave directly, and do not award earlier wave-clear scrap.
- **If wave is 0 or negative**: treat it as wave 1 for count, health, cover, and
  speed; the boss clause requires `wave >= 5`.
- **If a boss wave would have fewer than 3 regulars**: impossible under the
  current count floor; the elite scan still skips the boss.
- **If every regular rolls elite**: allowed; the chance is independent per
  zombie and capped, not quota-based.
- **If a dead player starts a new wave**: revive at 50% health; a living player
  is untouched.
- **If co-op is enabled without a gamepad**: player count falls back to 1, and
  composition uses the single-player count.
- **If both players die to contact damage on the frame the last zombie dies**:
  bullets resolve first, so the wave clears and contact damage is skipped that
  frame. An exploder fuse resolves before the clear and can end the run.
- **If a boss or elite dies as the last kill**: its guaranteed drops spawn, but
  the draft opens immediately and the next wave resets pickups. The drop is
  currently lost; this is a known limitation.
- **If the player never picks a card**: the game stays on the level-up screen;
  there is no timeout.
- **If the wave counter grows past 30**: the Victory screen appears **once**;
  endless play continues with regular HP frozen at wave-30 values, count
  capped by the live cap, elite chance at 25%, and boss HP still growing.
- **If a spawn would land in cover, within 40 px of another spawn, or within
  180 px of a living player**: reroll the position; if no valid edge position
  exists after 20 attempts, delay that spawn to the queue.
- **If a spitter is backed against the wall**: it retreats only to the playable
  boundary and keeps firing inward only with line of sight.

## Dependencies

| System | Type | Interface |
|---|---|---|
| Deterministic RNG | Hard | `spawnRng_` seeded from `runSeed_` and wave |
| Arena & Cover | Hard | `SetWave(wave, seed)`; playable rectangle; spawn safety |
| Enemy Roster & AI | Hard | Type stats, elite flag, boss scaling, AI contract |
| Combat Resolution | Hard | Kill counter, active fuses, wave-clear/game-over transitions |
| Upgrade Draft | Hard | `GenerateDraft()` on clear |
| Pickups | Soft | Spawned at wave start |
| Meta Progression | Soft | Wave reached, best wave, clear scrap, victory flag |
| HUD & Feedback | Soft | Banner, counters, boss bar, edge indicators |
| Audio & Music | Soft | Music and stingers per state |
| Local Co-op | Soft | Player count scales the horde; revive at wave start |
| Save & Persistence | Soft | Best wave and victory flag |

## Tuning Knobs

| Knob | Current | Safe Range | If Too High | If Too Low |
|---|---|---|---|---|
| Base wave count | 5 | 3–10 | Wave 1 overwhelms new players | Opening feels empty |
| Count growth per wave | 2 | 1–4 | Horde size outpaces DPS and caps | Pressure plateaus |
| Regular HP growth | `(w−1)²/12` | `(w−1)²/8`–`(w−1)²/16` | Enemies outpace player DPS | Late waves still one-shot |
| Co-op surcharge | `3 + floor(1.5w)` | 0–2× | Co-op becomes unfair | Co-op is easier than solo |
| Boss cadence | Every 5 | 3–10 | Boss fatigue, slow runs | Boss never feels special |
| Boss health growth | 18/wave + regular | 12–24/wave + regular | Boss outlasts ammo | Boss dies in one clip |
| Boss phase gate | 2.0 s, 2 adds | 1.0–3.0 s, 1–3 adds | Boss TTK too long | Boss TTK collapses |
| Speed variance | 0.91–1.0 | 0.85–1.0 | Visual desync from hitbox | Horde moves as one block |
| Elite start wave | 4 | 3–8 | Early spike | Elites arrive too late to matter |
| Elite chance | `0.05 + 0.015w`, cap 0.25 | 0–0.4 | Elite soup | Elites are forgettable |
| Arena growth | +10%/wave, cap 1800 | 5–15% | Player can outrun everything | Arena feels cramped |
| Cover count | `2 + w/3`, cap 12 | 0–12 | Maze-like, blocks shots | No positional play |
| Spawn activation | 0.6 s | 0.3–1.0 s | Spawns feel unresponsive | Telegraph is meaningless |

## Visual/Audio Requirements

- Wave banner: large "WAVE N" text for 2.4 s **inside the top HUD band**, never
  over the arena or spawn bands.
- Spawn rings: red pulsing circle around each zombie for 0.6 s; the ring gates
  activity.
- Off-screen spawn/threat edge pips, red for boss/elite.
- Boss: red aura ring plus top-screen health bar; no separate off-screen bar.
- Elite: a white pulsing rim/crown that multiplies the type tint; gold stays
  reserved for scrap/reward.
- Sprinter: an otherwise-unused saturated threat hue (hot magenta) plus a
  motion trail, never the cover grey-blue.
- Act banners at waves 9 and 21: "THE PRESSURE RISES" / "POWER TRIP".
- Cover: neutral grey-blue crates/barriers, distinct from floor.
- Corpses fade to ~35% alpha after 1.5 s and are y-sorted.
- Music: menu music on title, level-up music on draft, gameplay music during a
  wave (resumed, not restarted), stopped on game over/victory.
- Stinger: power-up sound at wave start; death sound on game over; victory
  fanfare at wave 30.

## UI Requirements

- HUD shows `WAVE N` and remaining zombie count.
- Boss health bar appears only while a boss is alive.
- Wave banner must not cover the play space for longer than 2.4 s.
- Victory screen shows wave, score, and the endless-continue option.
- The draft screen shows three cards, an optional fourth low-health Repair
  Kit, and a repair fallback; it accepts keys 1–4 plus gamepad A/B/X/Y.

## Acceptance Criteria

- **GIVEN** wave 1 solo, **WHEN** the wave starts, **THEN** exactly 5 zombies
  spawn.
- **GIVEN** wave 5 solo, **WHEN** the wave starts, **THEN** exactly 14 zombies
  spawn and exactly one is a boss.
- **GIVEN** wave 1 co-op, **WHEN** the wave starts, **THEN** 9 zombies spawn.
- **GIVEN** wave 1, **WHEN** composition is rolled, **THEN** no spitter,
  sprinter, or exploder appears.
- **GIVEN** wave 2+, **WHEN** composition is rolled many times, **THEN**
  spitters can appear; sprinters from wave 3 and exploders from wave 4, each
  capped at 40% of regular slots.
- **GIVEN** wave 4, **WHEN** the wave starts, **THEN** at least one non-boss
  zombie is elite.
- **GIVEN** wave 5, **WHEN** a chaser spawns, **THEN** it has 2 health; at wave 9
  it has 6; at wave 4 it has 1.
- **GIVEN** wave 9, **WHEN** a bloater spawns, **THEN** it has 10 health.
- **GIVEN** a wave-5 boss, **WHEN** it spawns, **THEN** it has 140 health,
  including the regular-scaling term.
- **GIVEN** a boss at 60% or 30% health, **WHEN** it crosses the threshold,
  **THEN** it is invulnerable for 2.0 s, summons 2 adds, and emits a shockwave.
- **GIVEN** a wave clear, **WHEN** the last zombie and fuses resolve, **THEN**
  the level-up screen opens and `5 + 2 × wave` base scrap is awarded before the
  bank multiplier.
- **GIVEN** a dead co-op player, **WHEN** the next wave starts, **THEN** that
  player revives at 50% health.
- **GIVEN** wave 10, **WHEN** the arena is built, **THEN** its side is 1300 px
  and it has 5 cover pieces.
- **GIVEN** wave 40, **WHEN** the arena is built, **THEN** its side is 1800 px
  and it has 12 cover pieces.
- **GIVEN** `--wave 5`, **WHEN** the game starts, **THEN** wave 5 begins without
  a loadout/draft and without earlier clear rewards.
- **GIVEN** a wave-1 spawn, **WHEN** it appears, **THEN** it is inactive for
  0.6 s and at least 180 px from a living player.
- **GIVEN** wave 31, **WHEN** wave 30 was cleared, **THEN** the Victory screen
  appeared once and endless play can continue with frozen regular HP.
- **GIVEN** an authored ammo faucet and a threat-budget profile run at waves
  5/10/20/30, **WHEN** measured, **THEN** average clear time is inside 10–40 s
  and frame time meets the Performance contract.
- **GIVEN** a boss at a 150 px cover gap, **WHEN** it chases through, **THEN**
  it passes without stalling.

Evidence: `tests/logic_tests.cpp` (`TestWaves`, `TestArena`,
`TestSpecialZombies`), smoke wave-1/2/5 checks, and the screenshots in
`build/smoke/`. New tests are required for the threat budget, spawn invariants,
elite/revive, clear scrap, victory, and performance caps.

## Implementation Deltas

- Count growth, boss add-instead-of-trade, co-op surcharge `3 + floor(1.5w)`,
  HP/boss/speed scaling, and the threat budget are revised design;
  `Waves.cpp`/`Zombie.h` still hold the old numbers.
- Spawns are active immediately and unchecked; add the 0.6 s gate, spacing,
  player-distance, and cover checks.
- Spitters retreat outside the arena, fire without LoS, and use the old
  150–250 band; clamp, gate, move to 350–500, and add the 0.3 s lead.
- Per-axis chase stalls behind a single crate; add the 0.5 s detour.
- The camera is unclamped and the arena is uncapped; add the 1800 px cap, camera
  clamp, floor culling, and edge indicators.
- Cover inset/spacing is 100/90 px; change to 150/150 px so the ~132 px boss
  can pass, and validate the body circle on spawn.
- Add the Loadout state, wave-30 Victory once, frozen regular HP in endless,
  and the act banners at waves 9/21.
- The live zombie, shockwave, and enemy-shot caps do not exist yet.
- Boss HP is `40 + 18w + 10×floor((w−1)²/12)`; add the two 2.0 s phase gates
  with 2 adds each.
- Build sprinter base is 120/30; target is 260/30. Build boss speed is 30/10;
  target is 90/30.
- Victory at wave 30 does not exist yet.
- Sprinter tint, banner placement, and corpse fading are art fixes.

## Open Questions

- Should the threat budget be validated by simulated play or by a headless
  DPS/clear-time model?
- Should boss waves rotate boss behaviours rather than reuse the bloater?
- Should elites be quota-based (exactly one) instead of one guaranteed plus a
  random chance?
- Should co-op use per-player drafts or keep the shared choice?
- Should endless mode plateau after wave 30, or keep scaling within the caps?
