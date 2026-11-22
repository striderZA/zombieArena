# Upgrade Draft

> **Status**: In Review — revised design 2026-11-22; implementation deltas pending
> **Author**: game-designer / systems-designer (documented from code)
> **Last Updated**: 2026-11-22
> **Last Verified**: source inspection 2026-11-22 (suites not executed)
> **Implements Pillar**: Build-crafting between fights

> **Source of truth**: `src/Upgrades.{h,cpp}`, `src/Game.cpp`
> (`GenerateDraft`, `ApplyUpgrade`, `NewRun`, `DrawLevelUpCards`),
> `src/Weapons.cpp`, `src/Player.cpp`, `src/Pickup.cpp`.
> This GDD is the **revised contract**; implementation deltas are listed at the
> end.

## Overview

The Upgrade Draft is the reward between waves: three cards drawn from a
weighted pool of permanent run upgrades, plus a Repair Kit fallback when the
pool runs dry. At least one card in every draft is **act-shaping** — a card that
changes how the run plays, not just a number. The pick is shared in co-op and
resets every run.

## Player Fantasy

The draft is the "one more wave" engine. For a few seconds the pressure stops,
and the player gets to say "what if I lean into fire rate this time?" The cards
should feel like tools for a plan, not random stat confetti: each pick should
visibly change how the next wave plays, and by mid-run the player should be able
to name their build ("piercing shotgun," "reload-on-kill rifle") without reading
a stat sheet.

## Detailed Design

### Core Rules

1. **Starting loadout and one-weapon lock.** A new run opens on a loadout
   choice: rifle, shotgun, or SMG if unlocked; the pistol is not a run option.
   The chosen weapon is the run's only weapon; the other weapons are not carried.
   Co-op players choose independently. The first upgrade draft opens after
   wave 1 is cleared. `--wave N` skips both.
2. **Draft size.** Three cards, plus an optional fourth low-health card. Cards
   are unique within a draft, except that a fallback or emergency Repair Kit is
   allowed to appear once; a picked card is removed from the pool before the
   next slot rolls.
3. **Eligible pool.** All upgrades with `weight > 0`, `stacks < maxStacks`,
   and relevance to the equipped weapon (Shotgun Choke is excluded unless the
   run carries the shotgun). The Repair Kit is excluded from the normal pool
   (weight 0).
4. **Weighted draw.** Each slot rolls uniformly over the pool's total weight and
   walks cumulative weights. Weights are relative, not percentages.
5. **Act-shaping guarantee.** At least **two** cards in each draft are drawn
   from the **build subset** (cards that change a verb, not a stat), while at
   least two build cards are eligible. If the first three slots contain fewer
   than two build cards, the missing slots are replaced with weighted draws
   from the eligible build subset. If only one build card is eligible, one is
   guaranteed; if none are eligible, the guarantee is vacuous. The build subset
   has 28 stacks, so at two per draft the guarantee lasts ~14 drafts; later
   drafts are stat/Repair drafts.
6. **Fallback.** If the pool cannot fill three cards, one Repair Kit is added at
   the end. A draft can therefore contain fewer than three cards: 1–2 pool
   cards plus one Repair Kit, or a single Repair Kit when the pool is empty.
7. **Application.** Selecting a card increments its run stack (when below the
   cap) and applies its effect to **both** co-op players. Health-only effects
   skip dead players; arsenal effects still apply.
8. **Run-scoped.** Stacks reset on a new run. They are not persistent and do
   not interact with Bunker meta levels except where the same stat is affected.
9. **Rarity is an odds band.** Card border colour reflects Common/Rare/Epic, and
   the bands correlate with weight (Common 80–100, Rare 55–75, Epic 25–35).
   Rarity does not independently modify odds.
10. **Draft determinism.** Drafts use a dedicated `draftRng_`, seeded from the
    run seed and wave, and are not consumed by combat noise. Given the same
    draft RNG state, `GenerateDraft` produces the same sequence. A restart with
    the same `--seed` deliberately uses a new run index and may differ.
11. **Repair Kit.** A repeatable +25 health fallback. It is only offered by the
    fallback path; it is not part of the normal pool. It is also offered as a
    fourth card when at least one **living** player is below 35% health, no
    Repair Kit is already in the draft, and the draft has fewer than four
    cards.

### Upgrade Catalog

| ID | Card | Rarity | Weight | Max stacks | Build card | Effect per stack |
|---|---|---|---|---|---|---|
| 0 | Rapid Fire | Common | 100 | 5 | No | Fire rate ×1.20 |
| 1 | Extended Mag | Common | 100 | 5 | No | Clip capacity +50%; added capacity goes to spare, not loaded |
| 2 | Vitality | Common | 100 | 5 | No | +10 max health, heal 10 if alive |
| 3 | Swift | Common | 100 | 5 | No | Move speed ×1.10 |
| 4 | Field Medic | Rare | 60 | 3 | No | Health pickup value +50% of base (25) |
| 5 | Scavenger | Rare | 60 | 3 | No | Ammo pickup value +50% of base (8); reload speed +10% |
| 6 | Hollow Point | Rare | 60 | 4 | No | +1 bullet damage |
| 7 | Steady Hands | Rare | 60 | 3 | No | Spread ×0.80 |
| 8 | Shotgun Choke | Rare | 55 | 3 | No | +1 shotgun pellet |
| 9 | Adrenaline | Epic | 25 | 5 | **Yes** | Heal +4 HP per kill |
| 10 | Bullet Storm | Epic | 25 | 2 | **Yes** | +1 projectile; single-projectile weapons pay 1 extra round per projectile, shotgun pellets are free |
| 11 | Repair Kit | Common | 0 | 999 | No | Heal 25 if alive (fallback/low-health only) |
| 12 | Piercing Rounds | Rare | 70 | 5 | **Yes** | Bullets pierce 1 additional zombie per stack |
| 13 | Ricochet | Epic | 30 | 3 | **Yes** | A bullet that hits cover bounces once per stack at 50% damage toward the nearest zombie within 300 px |
| 14 | Executioner | Rare | 55 | 5 | **Yes** | +25% damage to enemies below 35% health |
| 15 | Kill Reload | Rare | 55 | 5 | **Yes** | A kill loads 1 round per stack into the current clip |
| 16 | Explosive Rounds | Epic | 25 | 3 | **Yes** | Every 5th bullet explodes for 10 + 5×stacks damage in an 80 px radius (zombies only) |

Fresh-pool total weight: **980**. Build subset weight: **285 (29.1%)**. Four
flat commons: **400 (40.8%)**. The act-shaping guarantee, not the raw weights,
is what protects the pillar.

### States and Transitions

| From | To | Trigger |
|---|---|---|
| Title | Loadout | Start a new run |
| Loadout | Playing | Weapon chosen; wave 1 begins |
| Playing | LevelingUp | Wave cleared |
| LevelingUp | Playing | Valid card selected (keys 1–4, gamepad A/B/X/Y) |
| LevelingUp | LevelingUp | No card selected; the screen waits indefinitely |
| LevelingUp | Title / GameOver | Not reachable directly; back is ignored here |

### Interactions with Other Systems

| System | Direction | Interface |
|---|---|---|
| Deterministic RNG | Reads | Dedicated `draftRng_` seeded from run seed and wave |
| Weapons & Ammo | Writes | Fire rate, clip, reload, spread, damage, pellets, projectiles, pierce/ricochet/explosive |
| Player & Movement | Writes | Max health, heal, move speed |
| Pickups | Writes | Health/ammo pickup value and timer effects |
| Local Co-op | Writes | Applies the same effect to both players |
| HUD & Feedback | Reads | Card names, current→next values, rarity, draft size, gamepad glyphs |
| Wave Director | Reads | Draft opens on wave clear; blocks next wave until picked |
| Meta Progression | None directly | Bunker stats stack with run upgrades |
| Audio & Music | Writes | Level-up music, pick-confirmation cue |

## Formulas

### Draft selection probability

`pick_chance(card) = weight(card) / Σ weight(eligible cards)`

**Variables:** weight 25–100; pool 0–16 cards. For an empty pool the chance is
undefined; the fallback path handles it. **Output Range:** 0–100% depending on
the remaining pool. **Example:** fresh run, 16 eligible cards, total weight 980:
Bullet Storm = `25 / 980 ≈ 2.6%` per slot; Rapid Fire = `100 / 980 ≈ 10.2%`.

### Act-shaping guarantee

`if fewer than 2 build cards in slots 1–3: replace the missing slots with weighted_draw(eligible build subset)`

**Variables:** build subset weight 285 of 980 when all eligible.
**Output Range:** every draft contains ≥2 build cards while ≥2 are eligible;
≥1 while one is eligible. **Example:** a roll of Rapid Fire / Vitality / Field
Medic is replaced in slots 2–3 by two weighted build picks.

### Rapid Fire

`fire_rate = weapon.fire_rate × 1.20 ^ stacks`

**Variables:** stacks 0–5; output 1.0× to 2.49× base rate.

### Extended Mag

`clip = max(1, round(weapon.clip × (1 + 0.5 × stacks)))`

**Variables:** stacks 0–5; SMG output 30 → 105 rounds. The added capacity goes
to **spare ammo**, not the loaded clip; a reload loads it.

### Vitality / Swift

`max_health += 10 × stacks` (heal 10 per stack when alive)
`speed = base_speed × 1.10 ^ stacks`

**Variables:** stacks 0–5; output +50 max health and 1.61× speed at cap.

### Pickup upgrades

`health_pickup_value = 50 + 25 × field_medic_stacks`
`ammo_pickup_value = round((16 + 8 × scavenger_stacks) × weapon_ammo_share)`
`reload_time = weapon.reload_time / (1 + 0.10 × scavenger_stacks)`
`live_time = 6 + 0.6 × stacks`, `wait_time = max(1, 6 − 0.6 × stacks)`

**Variables:** Field Medic 0–3, Scavenger 0–3; SMG share 0.5. **Example:** Field Medic 3 = 125
health per pickup (capped by max health), 7.8 s live, 4.2 s wait.

### Damage, spread, pellets, projectiles

`bullet_damage += 1 × hollow_point_stacks`
`spread = weapon.spread × 0.80 ^ steady_hands_stacks`
`shotgun_pellets += shotgun_choke_stacks`
`all_projectiles += bullet_storm_stacks`; `ammo_per_shot = 1 + (weapon == shotgun ? 0 : bullet_storm_stacks)`
`heal_per_kill += 4 × adrenaline_stacks`

**Variables:** caps 4/3/3/5/2. Shotgun with all relevant caps: 6 + 3 + 2 = 11
pellets, costing 1 shell per shot at 2 Storm (shotgun pellets are free).

### Build cards

`pierce_count = piercing_stacks` (max 5)
`ricochet_count = ricochet_stacks` (max 3)
`executioner_multiplier = 1.25 ^ executioner_stacks` (max 5)
`kill_reload_rounds = kill_reload_stacks` (max 5)
`explosive_every = 5 if explosive_rounds else 0`; blast `10 + 5×stacks` damage / 80 px.

**Output Range:** pierce 0–5; ricochet 0–3; executioner ×1.00–3.05; kill reload
0–5 rounds; explosive every 5th bullet for 10–25 damage.

## Edge Cases

- **If the eligible pool is empty**: the draft contains exactly one Repair Kit.
- **If the pool has one card left**: the draft is that card plus one Repair Kit
  (two cards), because only one fallback is appended.
- **If the pool has two cards left**: three cards — two upgrades plus Repair.
- **If the build subset is empty but other cards remain**: the act-shaping
  guarantee is satisfied trivially; the draft proceeds without a build card.
- **If a card reaches its stack cap**: it leaves the pool for all later drafts
  in the run.
- **If a player is dead when an upgrade is applied**: they still receive
  arsenal effects; Vitality adds max health but does not heal; Repair Kit does
  nothing.
- **If both players are alive in co-op**: both receive every effect. Pickups are
  shared, so Field Medic/Scavenger upgrade the single shared pickup.
- **If Extended Mag is picked**: the new capacity is added to spare ammo, not
  the loaded clip; a reload loads it.
- **If the player is below 35% health in co-op**: only a **living** player
  qualifies; a dead player at 0% does not.
- **If a Repair Kit is already in the draft**: the low-health fourth card is not
  appended (no duplicate).
- **If the player has no valid input**: the draft never times out and the run
  cannot continue. There is no skip.
- **If `--wave N` is used**: no loadout or draft is shown at launch; the wave
  starts directly.
- **If a run restarts**: all upgrade stacks reset to zero.
- **If Repair Kit is selected repeatedly**: it can be taken again only when it
  is offered; it caps at 999 stacks for safety.
- **If a living player is below 35% health and no Repair Kit is present**: a
  fourth Repair Kit card is offered; keys 1–4 and gamepad A/B/X/Y select.
- **If a build card is drawn by the guarantee**: the slot still respects
  uniqueness and stack caps.

## Dependencies

| System | Type | Interface |
|---|---|---|
| Deterministic RNG | Hard | Dedicated `draftRng_` |
| Weapons & Ammo | Hard | Multipliers, reload, projectiles, pierce/ricochet/explosive |
| Player & Movement | Hard | Max health, heal, speed |
| Pickups | Soft | Pickup value/timer upgrades |
| Wave Director | Hard | Opens/closes the draft around waves |
| HUD & Feedback | Soft | Card layout, current→next values, rarity, input hints |
| Meta Progression | None | Same stats, applied independently; no interface call |
| Local Co-op | Soft | Shared pick, both-player application |
| Audio & Music | Soft | Level-up music, confirmation cue |

## Tuning Knobs

| Knob | Current | Safe Range | If Too High | If Too Low |
|---|---|---|---|---|
| Draft size | 3 | 2–4 | Choice paralysis; screen clutter | No meaningful choice |
| Rapid Fire | ×1.20, 5 stacks | ×1.10–1.30 | Fire rate trivializes ammo | Card feels invisible |
| Extended Mag | +50%, 5 stacks | +25–75% | Shotgun never reloads | Card is a minor reload saver |
| Vitality | +10 HP, 5 stacks | +5–20 | Player outheals the horde | Card is a trap |
| Swift | ×1.10, 5 stacks | ×1.05–1.15 | Kiting beats all AI | Card does not change decisions |
| Hollow Point | +1, 4 stacks | +1–2 | Bloater dies too fast | Damage pick feels weak |
| Steady Hands | ×0.80, 3 stacks | ×0.70–0.95 | Every weapon becomes a laser | Spread remains irrelevant |
| Shotgun Choke | +1, 3 stacks | +1–2 | Shotgun dominates close range | Shotgun stays a novelty |
| Adrenaline | +4 HP, 5 stacks | +2–8 | Sustain replaces pickups | Card is ignored |
| Bullet Storm | +1, 2 stacks | +1 | Every weapon doubles DPS | Epic pick feels flat |
| Build-card share | ≥2 per draft | 1–2 per draft | Draft feels scripted | Pillar fails |
| Repair Kit heal | 25 | 15–40 | Fallback sustains indefinitely | Fallback is useless |

## Visual/Audio Requirements

- Cards are vertically stacked, centered, with a key box (`1`/`2`/`3`), name,
  and description; show gamepad glyphs (`A`/`B`/`X`, plus `Y` for a fourth card).
- Each card shows the current stack count and the **current → next** value for
  multiplicative or non-obvious effects (fire rate, clip, speed, pickup value,
  reload).
- Description text is at least 20 px at 720p.
- Rarity colours: Common (off-white), Rare (blue), Epic (purple).
- Level-up music plays when the draft opens; a distinct confirmation cue plays
  on selection.
- The draft dims (does not replace) the battlefield behind it.
- The mouse cursor is hidden unless click/hover selection is implemented.
- A 0.3 s open lockout prevents a spammed weapon key from committing a card.

## UI Requirements

- Title: "WAVE N CLEARED" or "GET READY!" on the first draft.
- Subtitle: "CHOOSE AN UPGRADE".
- Bottom hint is dynamic: "Press 1–N" plus "or A/B/X/Y".
- Cards must fit at 720p and 1080p; the layout scales with window height.
- Gamepad A/B/X (and Y for a fourth card) selects the matching card.
- In co-op, the first valid press commits the shared choice; document the
  same-frame precedence (gamepad > keyboard; X > B > A).

## Acceptance Criteria

- **GIVEN** a fresh run, **WHEN** the first draft opens after wave 1, **THEN**
  exactly three unique cards are offered, at least two are build cards, and
  only cards relevant to the equipped weapon appear.
- **GIVEN** any draft with at least two eligible build cards, **WHEN** it opens,
  **THEN** at least two build cards are offered; with one eligible, at least
  one.
- **GIVEN** a card at its stack cap, **WHEN** later drafts are generated,
  **THEN** that card never appears.
- **GIVEN** the pool has fewer than three eligible cards, **WHEN** the draft
  opens, **THEN** one Repair Kit is appended and the draft may be shorter than
  three.
- **GIVEN** the pool is empty, **WHEN** the draft opens, **THEN** exactly one
  Repair Kit is offered.
- **GIVEN** a living player is below 35% health and no Repair Kit is present,
  **WHEN** a three-card draft opens, **THEN** a fourth Repair Kit card is
  offered; a dead player does not trigger it and a second Repair is never
  appended.
- **GIVEN** Extended Mag is picked, **WHEN** the effect applies, **THEN** clip
  capacity increases by 50%, the added capacity goes to spare ammo, and the
  loaded clip is unchanged.
- **GIVEN** Vitality is picked while a player is dead, **WHEN** it applies,
  **THEN** max health increases but health does not.
- **GIVEN** co-op, **WHEN** a card is picked, **THEN** both players receive the
  effect; the shared pickup is upgraded only once.
- **GIVEN** the same `draftRng_` state, **WHEN** `GenerateDraft` runs,
  **THEN** the same card sequence is produced.
- **GIVEN** a new run, **WHEN** it starts, **THEN** the loadout choice appears,
  only the chosen weapon is carried, and the first upgrade draft is not shown
  until wave 1 is cleared.
- **GIVEN** the draft screen, **WHEN** the player waits, **THEN** nothing
  advances until a valid card is picked.
- **GIVEN** a draft with two cards, **WHEN** the player presses key 3,
  **THEN** nothing happens.

Evidence: `tests/logic_tests.cpp` (`TestUpgradeTable`, `TestArsenal`,
`TestPlayerHealth`), smoke draft checks, and screenshot `02-levelup.png`. The
generation path needs a test seam (`GenerateDraft` over an `Rng&`/pool); new
tests are required for the cap, fallback shapes, build guarantee, co-op
application, and determinism.

## Implementation Deltas

- The catalog lacks the five build cards; add them and the two-build-card
  guarantee.
- The run currently carries all unlocked weapons; lock it to the loadout choice
  and make the eligible pool weapon-relevant.
- Bullet Storm extra projectiles are free; charge 1 extra round each.
- Extended Mag loads the added capacity; move it to spare.
- The low-health fourth card ignores dead players and can duplicate Repair;
  gate it as specified.
- `combatRng_` drives drafts and `runSeed_` mixes a run counter; add
  `draftRng_` and rewrite the AC.
- The first draft opens at wave 0; add the loadout screen and move the first
  draft after wave 1.
- Repair Kit is not offered at low health; add the fourth-card path.
- Cards show only `stacks/max`; add current→next values.
- The hint is static "Press 1-3", omits gamepad glyphs, and does not shrink for
  fewer cards.
- The battlefield is replaced by the menu backdrop, not dimmed; the cursor is
  visible but inert; there is no open lockout.
- Scavenger does not affect reload speed yet.

## Open Questions

- Should the draft reroll once per run?
- Should rarity ever influence weight independently?
- Should co-op draft per player instead of sharing one choice?
- Should the post-40-stack endgame be a repeatable pool, a victory wave, or an
  ascension/modifier layer? (Wave 30 victory owns the current terminus.)
