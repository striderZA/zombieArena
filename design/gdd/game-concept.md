# Game Concept: Zombie Arena

*Created: 2026-11-21*
*Status: Draft (reverse-documented from the current implementation)*

> This document is the **revised design contract** (2026-11-22); implementation
> deltas are listed in the system GDDs and are not yet in `src/`.
> It was reconstructed from `README.md`, `docs/raylib-port.md`, the source, and
> the test suites. Numeric claims marked as revised design are not yet traceable
> to code; see the systems
> index and the individual GDDs for detail.

## Core Identity

| Field | Details |
|---|---|
| Working title | Zombie Arena |
| Genre | Top-down twin-stick arena survival / wave shooter |
| Platform | Windows desktop (raylib 6.2, C++17); the code is portable to other desktop platforms |
| Estimated scope | Small — a complete playable vertical slice; remaining work is content, balance, and hardening |
| Engine recommendation | raylib 6.2 (already implemented; 2D rendering, raudio, no physics engine) |

## Elevator Pitch

Survive escalating zombie waves in a walled arena, choosing one of three run
weapons (rifle, shotgun, or SMG if unlocked), using cover, and picking a
three-card upgrade draft between waves.
Every death converts into scrap
that buys permanent bunker upgrades, so each run starts a little stronger.

## Core Fantasy & Unique Hook

- **Core fantasy**: Turn a desperate last stand into a power trip. The first
  wave is a scramble; by wave twenty-one you are a heavily upgraded killing machine
  cutting through a horde you could not have survived ten minutes earlier.
- **Unique hook**: Like *Vampire Survivors*' build-crafting and *Robotron*'s
  twin-stick pressure, but with explicit cover, limited ammo, and a bunker meta
  loop that makes every failed run pay forward.

## Pillars

| Pillar | One-sentence definition | Design test |
|---|---|---|
| Escalating, readable pressure | Each wave adds a little more than the player can comfortably handle, but every threat is telegraphed by silhouette, tint, and sound. | When choosing between a surprise attack and a visible wind-up, choose the visible wind-up. |
| Build-crafting between fights | The run's identity comes from draft choices and weapon selection, not from a fixed loadout. | When choosing between a flat stat increase and a choice that changes how the run plays, choose the choice. |
| Every death pays forward | A failed run still advances the account through scrap and permanent upgrades. | When choosing between punishing a death and rewarding the attempt, keep the scrap. |

### Anti-Pillars

- We will NOT add a story campaign or dialogue tree — it would dilute the
  pick-up-and-play loop and the arcade score chase.
- We will NOT add stealth, puzzles, or environmental manipulation — combat
  readability and fast restarts are the priority.
- We will NOT add PvP, online co-op, or matchmaking — local co-op is the
  intended social mode and networking would add a dependency and a test burden.

## Core Loop

### 30-Second Loop

Aim with the mouse or right stick, fire, reload before the clip runs dry,
kite the nearest zombie, break line of sight around cover, and grab the health
or ammo drop before it expires. Feedback is immediate: muzzle flash, knockback,
damage numbers, blood decals, and screen shake.

### 5-Minute Loop

Clear a wave → choose one of three upgrade cards (plus a Repair Kit fallback,
and a fourth emergency card below 35% health) → face a larger, faster, more
varied wave. Every fifth wave adds a boss with a health bar and guaranteed
drops on top of a normal wave (unless that kill clears the wave; the drop is
currently lost, a known limitation). The player is constantly deciding whether to
spend ammo now or reposition, and whether a draft pick patches a weakness or
leans into a strength.

### Session Loop (30–120 minutes)

Title → run → death or a new personal best → Bunker → retry. A run is short
enough that "one more" is cheap. High score, best wave, kills, and lifetime
scrap persist between sessions.

### Progression Loop (First Few Sessions)

Scrap earned per kill and per wave buys five permanent upgrades (max health,
bullet damage, move speed, starting spare ammo, scrap gain), a one-time SMG
blueprint, and a cosmetic Veteran Badge capstone. Progression is account-wide,
deterministic in cost, and capped, so the long-term goal is mastery and score
rather than an infinite stat treadmill. A competent player maxes the Bunker in
roughly 5–8 runs; the hi-score is a progression score because Bunker power can
raise kill volume.

## MVP Definition

The smallest build that tests the loop: one arena, rifle/shotgun/SMG,
three zombie types, five waves, a three-card draft (plus fallback) between
waves, death and restart, and a persisted high score. The current build already exceeds this.

## Scope Tiers

| Tier | Included scope | Target timeline / team |
|---|---|---|
| MVP | 5 waves, 3 weapons, 3 enemy types, draft, death/restart, high score | Solo, already implemented |
| Vertical Slice | 7 enemy types + elites, boss every 5 waves, cover, pickups, meta/bunker, local co-op, settings, full HUD | Solo; design revised 2026-11-22, implementation deltas pending |
| Full Vision | More arena themes, additional weapons and enemy behaviours, run modifiers/difficulty tiers, achievements, accessibility passes | Solo, not scheduled |

## Risks

| Risk | Type (technical / design / market) | Mitigation or validation |
|---|---|---|
| Content repetition sets in after the first few runs | Design | Elite variants, boss cadence, and draft synergies are the current counters; run modifiers are the next candidate |
| Upgrades or weapons collapse into one dominant strategy | Design | Deterministic logic tests pin current numbers; balance-check/review passes are required for changes |
| Large zombie counts or bullets cost too much frame time | Technical | Explicit pools (`160` bullets, `120` decals), live caps (96 solo / 128 co-op), arena cap 1,800 px, floor culling, and a `--profile` smoke pass to add |
| Save-file edits trivialize progression | Technical / design | Profile values are clamped on load; tamper resistance is out of scope for a single-player game |
| Co-op requires a physical gamepad for player 2 | Technical | Documented limitation; gamepad input is scripted in the smoke suite |

## Visual Identity Anchor

- **Direction name**: Grimy arcade survival
- **One-line visual rule**: The floor is muted asphalt and sickly green, so
  enemies, drops, and danger colours are the only saturated things on screen.
- **Supporting visual principles and design tests**:
  - Silhouette first — every enemy type must be identifiable at a glance by
    shape and scale, not colour alone.
  - Threat uses warm reds/oranges; reward uses yellow/green; cover is neutral
    grey-blue. Do not use reward colours for hazards.
  - Feedback is exaggerated and brief — hit flash, knockback, and shake sell
    impact without covering the play space.
- **Color philosophy**: Desaturated environment, high-contrast actors. Red is
  reserved for damage and bosses, white rims/crowns for elites, and gold for
  scrap-value moments,
  green for spitters and healing.

Known limitation: the current build reuses the chaser texture for spitters and
sprinters, and the bloater texture for exploders and the boss. Those pairs are
distinguished by tint and scale, which does not yet meet the silhouette-first
rule. Distinct silhouettes are an open art task.

## MDA Analysis

- **Primary aesthetic**: Challenge
- **Desired player emotion**: Focused tension that resolves into empowerment
  after a strong draft or a clean wave.
- **Dynamics that create it**: Kiting and cover use, ammo conservation, target
  priority (spitters/exploders/elites), risk-reward around pickups.
- **Mechanics that enable those dynamics**: Twin-stick aiming, per-weapon ammo
  pools, wave scaling, weighted drafts, contact/projectile/explosion damage,
  boss and elite modifiers.

## Player Motivation Profile

- **Autonomy**: Weapon choice at the loadout (one weapon per run), draft choice,
  positioning, and when to spend
  scrap in the Bunker.
- **Competence**: Aim, reload discipline, kiting, and learning enemy tells;
  visible in wave reached, score, and hi-score.
- **Relatedness**: Local co-op with a shared score; the Bunker as a persistent
  home base between runs.
- **Primary player type**: Players who enjoy arcade score chasing and build
  experimentation in short sessions.
- **Secondary appeal**: Co-op pairs looking for a drop-in couch shooter.
- **Not for**: Players seeking narrative, exploration, stealth, or competitive
  multiplayer.
- **Market validation**: *Vampire Survivors* (draft dopamine), *Robotron 2084*
  and *Nation Red* (twin-stick arena pressure), *Call of Duty: Zombies*
  (wave/bunker loop). The differentiator is cover + limited ammo rather than
  pure bullet-heaven.

## Flow State Design

- **Challenge-skill balance**: Wave size, the per-wave speed bonus, and regular
  enemy health (`(wave − 1)² / 12`) grow every wave; enemy variety expands
  through wave 4 (spitter, sprinter, exploder). The draft lets the player match
  that growth. Permanent upgrades provide a floor after repeated failures. The
  run has a three-act arc (desperation, tension, power trip) and a Victory
  milestone at wave 30; endless play continues after it.
- **Clear goals**: The HUD names the current wave, remaining zombies, score,
  hi-score, and boss health. The wave banner and level-up screen mark every
  transition.
- **Feedback**: Damage numbers, hit flash, knockback, blood decals, shockwaves,
  screen shake (toggleable), and a damage vignette.
- **Agency and focus**: The player chooses the weapon, the draft card, the
  firing lane, and the Bunker purchase; no forced cutscenes or downtime.

## Open Questions

- Should spitter/sprinter and exploder/boss get distinct silhouettes instead of
  tint-and-scale variants?
- Is ammo actually scarce enough to matter, or should the concept drop the
  "limited ammo" claim?
- Should difficulty tiers or run modifiers become part of the full vision?
- Should cover become destructible, or remain permanent per wave?
- How much content variety is enough before the game needs a second arena
  theme or a second boss?
- Is local co-op worth expanding (shared revive, per-player drafts) or should
  it stay a secondary mode?
