# Economy System (Score, Scrap & Bunker)

> **Status**: In Review — revised design 2026-11-22; implementation deltas pending
> **Author**: economy-designer / systems-designer (documented from code)
> **Last Updated**: 2026-11-22
> **Last Verified**: source inspection 2026-11-22 (suites not executed)
> **Implements Pillar**: Every death pays forward

> **Source of truth**: `src/Meta.{h,cpp}`, `src/Game.cpp`
> (`AwardScrap`, `FinishRun`, `NewRun`, `ApplyMetaToSlot`, `BuyMeta`),
> `src/Zombie.h` reward values. Persistence rules are in `LoadProfile` /
> `SaveProfile`. This GDD is the **revised contract**; implementation deltas are
> listed at the end.

## Overview

The economy has two currencies with different jobs. **Score** is per-run
bragging rights and the hi-score chase; it resets each run and is never spent.
**Scrap** is persistent progression: earned from kills and wave clears, banked
at the end of a run with a depth multiplier, and spent in the Bunker on five
capped permanent upgrades plus a one-time SMG blueprint. The system also owns
profile stats (runs, kills, best wave, lifetime scrap) and the save/load rules
that keep them honest.

## Player Fantasy

Death should sting for a second, then turn into a plan. The Bunker is the
"next time" screen: the player sees the scrap their failure earned, buys one
concrete improvement, and immediately wants to try again. The **first** Bunker
visit must offer at least one affordable improvement. Score is the opposite
feeling — a measure of how well the run went, shaped by depth, wave clears, and
flawless play, safe from the Bunker economy.

## Detailed Design

### Core Rules

1. **Score is per-run and skill-shaped.** Kills add score immediately. Wave
   clears add `50 × wave`; a wave cleared without **losing health** adds a
   further `100 × wave` (absorbed hits do not break it); bosses add `50 × wave`
   on top of their base value; clearing wave 30 adds `victory_bonus = 5,000`.
   The in-memory hi-score updates as soon as the run score exceeds it. Score is
   never spent. Because Bunker power can raise kill volume, the hi-score is a
   progression score; a separate no-Bunker board is out of scope.
2. **Scrap is persistent.** Kills and wave clears add to a run bank
   (`scrapRun`). At `FinishRun` the bank is multiplied by the Scrap Magnet bank
   multiplier and the depth multiplier, then transferred to the profile.
3. **Scrap is never lost on death.** There is no death penalty, corpse run, or
   banking choice. `FinishRun` is idempotent per run. The depth multiplier
   rewards going deeper; it does not punish dying shallow. The **first completed
   run on a profile** grants a one-time `first_run_stipend = 10` scrap, so the
   first Bunker visit always has an affordable row even after a zero-kill
   wave-1 death.
4. **Kill rewards.** Score and scrap values depend on enemy type and elite
   status (see combat GDD). Scrap is `max(1, round(base × (elite ? 3 : 1)))`.
   The boss uses `25 + 5 × wave` scrap and `100 + 50 × wave` score. The Magnet
   and depth multipliers are applied once at the bank, not per kill.
5. **Wave-clear reward.** Clearing wave `N` awards `5 + 2 × N` base scrap to the
   run bank.
6. **Bunker upgrades are capped.** Five upgrades with fixed per-level costs.
   A maxed upgrade reports cost `-1` and can never be purchased again.
7. **SMG blueprint is one-time.** 250 scrap unlocks the SMG for every future
   run; buying it twice is impossible.
8. **Purchases are immediate, atomic, and saved.** A successful Bunker purchase
   writes the profile atomically (temp file + rename) and plays the power-up
   sound. A failed write rolls back the in-memory purchase and plays the
   failure sound. A failed affordability check changes nothing.
9. **Run stats.** Finishing a run increments `runs`, adds `kills`, updates
   `bestWave` to the maximum **wave reached**, records `wavesCleared` for the
   depth multiplier, grants the first-run stipend once, and adds the banked
   scrap to `scrap` and `totalScrap` (or only `totalScrap` after the Bunker is
   fully maxed).
10. **Profile clamps and recovery on load.** Negative counters become 0; meta
    levels clamp to their max; counters clamp to `[0, 1_000_000_000]`; unknown
    keys are ignored; a malformed line is skipped without discarding later
    keys. A missing file leaves defaults.
11. **Terminus and post-cap.** Clearing wave 30 triggers Victory and the
    victory bonus. Endless play may continue; the Bunker remains capped. "Post-
    cap" means **after every Bunker row is maxed**: from then on, banked scrap
    is recorded as lifetime scrap/scoreboard only. Before that, endless scrap
    remains spendable.
12. **Bunker reachability.** The Bunker is reachable from Title and Game Over
    only. There is no mid-run purchase path; a purchase always applies to the
    next run.

### Pacing Budget

The intended shape is **multi-run, short-session, skill-compressed**. A
competent player who dies around wave 8–10 should max the Bunker in roughly
**5–8 runs**; a new player dying on wave 3 in roughly **65 runs** (wave 2 is
~118); an expert at wave 20–30 in **1–2 runs**. The depth multiplier makes
deeper runs strictly better per run, so shallow farming is not the optimal
strategy.

Expected solo faucet (Magnet 0, expected composition/elite rolls; base scrap
before the bank multipliers):

| Waves cleared | Base scrap | Depth × | Effective bank | Runs to 3610 |
|---|---|---|---|---|
| 3 | 50.6 | 1.09 | 55.2 | 65.4 |
| 5 | 170.5 | 1.15 | 196.1 | 18.4 |
| 10 | 521.1 | 1.30 | 677.4 | 5.3 |
| 15 | 968.0 | 1.45 | 1,403.6 | 2.6 |
| 20 | 1,673.0 | 1.60 | 2,676.8 | 1.3 |
| 30 | 3,589.0 | 1.90 | 6,819.1 | 0.5 |

With Magnet 5 the effective bank doubles: wave 10 = 1,354.8 (2.7 runs), wave
15 = 2,807.2 (1.3 runs). The table is a first-pass model; the balance pass
validates it against measured clear times.

### Bunker Catalog

Rows are ordered by expected value. Keys 1–7 select the first seven rows;
confirm buys the selected row; Back exits.

| Row | Upgrade | Effect per level | Max | Base cost | Cost step | Total to max |
|---|---|---|---|---|---|---|
| 1 | Reinforced Vest | +5 starting max health | 5 | 10 | 45 | 500 |
| 2 | Weapon Tuning | +1 bullet damage | 2 | 120 | 120 | 360 |
| 3 | SMG Blueprint | Unlocks the SMG | 1 | 250 | — | 250 |
| 4 | Running Shoes | +3% move speed (multiplicative, every direction) | 5 | 10 | 45 | 500 |
| 5 | Scrap Magnet | +20% scrap bank multiplier | 5 | 50 | 50 | 750 |
| 6 | Deep Pockets | +10% starting spare ammo | 5 | 50 | 50 | 750 |
| 7 | Veteran Badge | One-time cosmetic gold nameplate/title; no stat effect | 1 | 500 | — | 500 |
| 8 | Back | Leaves the Bunker | — | — | — | — |

Total cost to buy every permanent upgrade and the capstone: **3,610 scrap**.

**Catalog rule:** no paid row may be strictly dominated by a free draft card.
Paid rows are a permanent floor that stacks with run upgrades, not a
replacement. The balance pass must re-check this rule whenever draft values
change.

### States and Transitions

| From | To | Trigger |
|---|---|---|
| Run | Run bank | Kill, wave clear, flawless bonus |
| Run bank | Profile | `FinishRun` (death, quit, shutdown) |
| Bunker | Profile | Successful purchase; written atomically |
| Profile | Run start | `ApplyMetaToSlot` copies levels into stats |
| Profile | Disk | Purchase, run finish, shutdown |

### Interactions with Other Systems

| System | Direction | Interface |
|---|---|---|
| Save & Persistence | Writes | User-writable profile path; atomic write; recovery |
| Combat Resolution | Reads | Kill score/scrap values and elite multipliers |
| Wave Director | Reads | Wave reached, waves cleared, clear scrap, victory flag |
| Player & Movement | Writes | Starting max health, move speed |
| Weapons & Ammo | Writes | Bullet damage, starting spare ammo, SMG unlock |
| Pickups | None | Pickup values are draft-only; Deep Pockets affects starting spare |
| Upgrade Draft | None | Run upgrades stack independently with meta levels |
| HUD & Feedback | Reads | Score, hi-score, run scrap, total scrap, stats |
| Menus & Settings | Reads/writes | Bunker row state, purchase input |
| Local Co-op | Reads | Shared profile and purchases apply to both players |
| Audio & Music | Writes | Purchase success/failure cues |

## Formulas

### Scrap from a kill

`scrap = max(1, round(base_scrap(type, wave) × (elite ? 3 : 1)))`

`base_scrap(type, wave) = scrap_value(type)` normally;
`base_scrap(boss, wave) = 25 + 5 × wave`.

**Variables:**

| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| Base scrap | `scrap_value` | int | 1/2/25 | 1 default, 2 special, 25 boss base |
| Boss depth term | `5 × wave` | int | 5+ | Boss only |
| Elite | `elite` | bool | true/false | ×3 when elite |

**Output Range:** 1 to 175 per kill before bank multipliers, floored at 1.
**Example:** normal chaser = 1; elite spitter = 6; wave-30 boss = 175.

### Scrap from a wave clear

`scrap = max(1, 5 + 2 × wave)`

**Output Range:** 7 at wave 1 to 65 at wave 30. **Example:** wave 10 = 25.

### Bank multiplier

`banked = round(scrapRun × magnet_multiplier × depth_multiplier)`

`magnet_multiplier = 1 + 0.20 × scrap_magnet_level` (0–5 → 1.0–2.0)
`depth_multiplier = 1 + 0.03 × min(waves_cleared, 30)`

**Output Range:** 1.0–2.0 × 1.0–1.90 = up to 3.8× at wave 30, Magnet 5; the
depth term is capped, so endless play does not inflate the multiplier.
**Example:** 400 base scrap, wave 10 cleared, Magnet 5 →
`round(400 × 2.0 × 1.30) = 1,040`.

### Bunker upgrade cost

`cost(level) = base_cost + cost_step × level`, or `-1` when `level ≥ max_level`

**Output Range:** 10 to 250 depending on upgrade; `-1` at max.
**Example:** Vest at level 3 costs `10 + 45 × 3 = 145`; at level 5, `-1`.

### Meta effects at run start

`max_health = 100 + 5 × vest_level`
`speed = 200 × 1.03 ^ shoes_level` (every direction; input normalized)
`bullet_damage = 1 + tuning_level` (before run upgrades)
`starting_spare = base_spare × (1 + 0.10 × pockets_level)`

**Variables:** Vest 0–5, Shoes 0–5, Tuning 0–2, Pockets 0–5.
**Output Range:** 100–125 HP, 200–231.9 px/s from Shoes alone (capped at 300
when combined with draft Swift by the combat contract), +0–2 damage, 1.0–1.5×
starting spare. **Example:** Vest 3, Shoes 5, Tuning 1, Pockets 2 → 115 HP,
`200 × 1.1593 ≈ 232` px/s, 2 base damage, 1.2× spare.

### Run finish

`banked = round(scrapRun × magnet_multiplier × depth_multiplier)`
`profile.totalScrap += banked`
`if not bunker_maxed: profile.scrap += banked`
`profile.runs += 1`
`profile.kills += runKills`
`profile.bestWave = max(profile.bestWave, waveReached)`
`profile.wavesClearedTotal += wavesCleared`
`if fresh_profile: profile.firstRunStipendClaimed = true; profile.scrap += 10`

**Variables:** run bank, waves cleared, wave reached, Magnet level, Bunker
state, first-run flag. **Output Range:** any non-negative integer up to the 1e9
clamp. **Example:** 400 base, wave 10, Magnet 0 → bank 520; profile scrap and
lifetime scrap each +520, runs +1.

## Edge Cases

- **If the player dies on wave 1 before clearing it**: they bank 0–5 kill scrap
  plus the one-time 10-scrap first-run stipend, so at least one row is
  affordable. Clearing wave 1 and dying on wave 2 banks 28.8.
- **If the first-run stipend was already claimed**: a zero-kill wave-1 death
  banks 0; the guarantee applies only to the first completed run.
- **If the player buys before earning any scrap**: nothing changes and the
  failure sound plays.
- **If the player quits mid-run**: `Shutdown` calls `FinishRun` when a run is
  active, so the bank and stats are saved.
- **If the player quits before wave 1 starts**: `runActive` is false, so no run
  is counted and no bank is transferred.
- **If `FinishRun` is called twice**: the second call is a no-op.
- **If a purchase is unaffordable**: nothing changes and the failure sound
  plays.
- **If a purchase write fails**: roll back the in-memory grant, play the failure
  sound, and retry at shutdown.
- **If an upgrade is maxed**: cost is `-1`; the row shows MAXED and cannot be
  bought.
- **If the SMG is already unlocked**: the blueprint purchase is rejected.
- **If the profile file is missing**: all values default to zero, with the SMG
  locked and every meta level at 0.
- **If a profile line is malformed or has an unknown key**: skip that line;
  later known lines still load.
- **If the profile contains negative values**: clamp them to 0.
- **If the profile contains values above 1e9**: clamp to 1e9.
- **If a meta level exceeds its max in the file**: clamp to the max.
- **If a kill awards less than 1 after rounding**: floor at 1.
- **If a boss is also elite**: impossible; bosses are never elite.
- **If the player reaches wave 30**: Victory triggers once; endless play may
  continue. Regular HP freezes at wave-30 values, depth is capped at 1.9, and
  post-cap scrap is lifetime-only only after every Bunker row is maxed.
- **If the player buys an upgrade while the Bunker is open from Title/Game
  Over**: the purchase applies to the next run. There is no mid-run path.
- **If the profile is read-only**: the game still starts; purchases roll back
  and a visible warning is shown.

## Dependencies

| System | Type | Interface |
|---|---|---|
| Save & Persistence | Hard | User-writable profile path, atomic write, recovery, clamps |
| Combat Resolution | Hard | Score/scrap values and elite multipliers |
| Wave Director | Hard | Wave number, waves cleared, clear reward, victory flag |
| Player & Movement | Hard | Health/speed meta effects |
| Weapons & Ammo | Hard | Damage meta effect, starting spare, SMG unlock |
| Pickups | None | Pickup values are draft-only |
| HUD & Feedback | Soft | Score, scrap, stats, purchase feedback |
| Menus & Settings | Soft | Bunker navigation and purchase input |
| Local Co-op | Soft | Shared profile and purchases |
| Audio & Music | Soft | Purchase success/failure cues |

## Tuning Knobs

| Knob | Current | Safe Range | If Too High | If Too Low |
|---|---|---|---|---|
| Base kill scrap | 1/2/25 | 1–5 / 2–6 / 15–40 | Bunker completes too fast | Grind outweighs play |
| Boss scrap term | `25 + 5w` | `25 + 3w`–`25 + 8w` | Boss waves dominate income | Boss feels unrewarding |
| Wave-clear scrap | `5 + 2w` | `3 + 1w` to `10 + 4w` | Idle wave farming dominates | Clearing feels unrewarded |
| Depth multiplier | +3%/wave, cap 1.9 | +2–4%/wave, cap 1.6–2.2 | Deep runs trivialize the sink | Shallow farming returns |
| First-run stipend | 10 | 5–20 | First Bunker visit trivialized | Guarantee fails |
| Bunker capstone | 500 | 250–1,000 | Capstone unreachable | No post-cap goal |
| Scrap Magnet | +20%/level, max 5 | +10–30% | Economy inflates | Upgrade is a trap |
| Vest/Shoes first level | 10 | 5–20 | First Bunker visit is empty | Trivializes the first purchase |
| Tuning cost | 120 + 120/level | 80–200 | Damage upgrade unreachable | Permanent power spike too cheap |
| SMG cost | 250 | 200–400 | Unlock arrives after it matters | Trivializes early waves |
| Max meta level | 2–5 | 1–8 | Permanent power trivializes endless | No long-term goal |

Boss HP and phase gates are owned by `wave-system.md`; boss scrap/score are
unchanged by that balance pass.

## Visual/Audio Requirements

- Bunker: dark overlay, centered rows ordered by value, selected row outlined
  in yellow.
- Currency: scrap shown in gold; MAXED and unaffordable states visually
  distinct; unaffordable uses dim amber, never the damage red.
- Purchase success: power-up sound; failure: reload-failed sound.
- Title: profile stats (scrap, runs, kills, best wave, waves cleared).
- Game over: run score, hi-score, wave, scrap earned (the **banked** amount),
  total scrap.
- HUD: score top-left, hi-score top-right; a small gold **base** run-scrap
  counter and "+N" popups. The game-over screen shows the banked amount with
  the Magnet/depth multipliers applied.
- Victory: wave-30 banner, score, and endless-continue option.

## UI Requirements

- Bunker rows in the catalog order: Vest, Tuning, SMG, Shoes, Magnet, Pockets,
  Veteran Badge, Back.
- Each row shows name, description, current/max level, and cost or MAXED.
- Keys 1–7 select rows 1–7; confirm buys the selected row; back exits.
- Title and game-over screens expose the persistent stats without a separate
  save menu.
- Purchase uses a single confirm action, not a modal confirmation dialog;
  feedback is immediate.
- `bestWave` means the highest wave reached; the game-over screen also shows
  waves cleared.

## Acceptance Criteria

- **GIVEN** a normal chaser kill, **WHEN** Magnet is 0 and wave is 1, **THEN**
  the run bank increases by 1 and score by 10.
- **GIVEN** an elite spitter at wave 10, **WHEN** it is killed, **THEN** score is
  45 and the run bank increases by 6.
- **GIVEN** wave 5 cleared, **WHEN** Magnet is 0, **THEN** base scrap added is
  15.
- **GIVEN** a wave-30 boss kill, **WHEN** it is killed, **THEN** score is
  `100 + 1,500 = 1,600` and base scrap is `25 + 150 = 175`.
- **GIVEN** Magnet 5 and wave 10 cleared, **WHEN** `FinishRun` resolves,
  **THEN** the bank is `round(scrapRun × 2.0 × 1.30)`.
- **GIVEN** a Vest at level 0, **WHEN** the player has 10 scrap and buys it,
  **THEN** scrap becomes 0 and level becomes 1.
- **GIVEN** a Vest at level 5, **WHEN** the row is queried, **THEN** cost is
  `-1` and no purchase is possible.
- **GIVEN** 250 scrap and a locked SMG, **WHEN** the blueprint is bought,
  **THEN** the SMG unlocks and 250 scrap is spent.
- **GIVEN** a run ends with 42 scrap and 3 waves cleared, Magnet 0, **WHEN**
  `FinishRun` resolves, **THEN** banked scrap is `round(42 × 1.09) = 46`, and
  profile scrap and lifetime scrap both increase by 46 and runs by 1.
- **GIVEN** a corrupted/negative profile, **WHEN** it loads, **THEN** negatives
  clamp to 0, over-max levels clamp to max, over-1e9 counters clamp to 1e9, and
  the game still starts.
- **GIVEN** a profile with a malformed line, **WHEN** it loads, **THEN** that
  line is skipped and later known keys still load.
- **GIVEN** a successful purchase, **WHEN** the game restarts, **THEN** the
  purchase persists.
- **GIVEN** a fresh profile and a wave-1 death with zero kills, **WHEN** the
  Bunker opens, **THEN** the 10-scrap first-run stipend makes at least one row
  affordable.
- **GIVEN** the first-run stipend was already claimed, **WHEN** a zero-kill
  wave-1 death ends, **THEN** no second stipend is granted.
- **GIVEN** a read-only profile path, **WHEN** the player buys an upgrade,
  **THEN** the purchase rolls back and a visible warning appears.
- **GIVEN** wave 30 cleared, **WHEN** the run ends or continues, **THEN**
  Victory and the 5,000 score bonus are recorded once, depth is capped at 1.9,
  and post-cap scrap counts only toward lifetime after every Bunker row is
  maxed.
- **GIVEN** a fully maxed Bunker, **WHEN** a run ends, **THEN** banked scrap
  increases lifetime scrap but not spendable scrap.
- **GIVEN** a Veteran Badge purchase, **WHEN** 500 scrap is spent, **THEN** the
  badge is owned and the Bunker total is 3,610.
- **GIVEN** a post-bank kill, **WHEN** it lands after Game Over, **THEN** no
  score or scrap is awarded and the game-over display matches the banked
  amount.

Evidence: `tests/logic_tests.cpp` (`TestMeta`), smoke checks for the bunker
purchase (10 → 0 with Vest level 1; SMG 250 → 0), cross-instance persistence,
and `12-bunker.png`. The clamp, malformed, overflow, depth-bank, first-death
affordability, and victory paths need new tests.

## Implementation Deltas

- The profile lives in `assets/gamedata`; move to a user-writable path and add
  atomic temp+rename writes.
- `SaveProfile` results are ignored at `BuyMeta` and `FinishRun`; check and
  surface failures, with rollback on purchase.
- `LoadProfile` aborts on the first malformed token; parse line-by-line.
- Counters have no upper clamp; add the 1e9 clamp and saturating add.
- Magnet is applied per award; move it to the bank multiplier and add the
  capped depth multiplier.
- Add the one-time 10-scrap first-run stipend and the `firstRunStipendClaimed`
  profile flag.
- Add the 5,000 Victory score bonus and the Veteran Badge capstone (total
  3,610).
- `bestWave` currently uses waves cleared; switch to wave reached and record
  both.
- Kill scrap lacks the boss `25 + 5w` override; add it.
- Score lacks wave-clear, flawless, and boss bounty terms; add them.
- Vest/Shoes first level is 40; lower to 10 and set step 45.
- Deep Pockets affects ammo pickups; change to starting spare ammo.
- The Bunker is reachable only from Title/Game Over; the doc now matches.
- Wave-30 Victory and `wavesCleared` do not exist yet.

## Open Questions

- Should there be a prestige/reset layer after all upgrades are maxed?
- Should scrap have a cosmetic or difficulty-unlock sink?
- Should the player choose to bank or gamble scrap at the end of a run?
- Should difficulty tiers multiply scrap gain?
- Should co-op split the bank or keep the shared-account accelerant?
