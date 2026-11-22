# Quick Design Spec: Balance Pass — Pistol, Boss TTK, Threat Model, Kiting

**Type**: Tuning + Addition
**Systems**: Combat (SYS-09), Wave Director (SYS-10), Economy (SYS-13)
**GDD Reference**: `design/gdd/combat-system.md`, `design/gdd/wave-system.md`,
`design/gdd/economy-system.md`
**Date**: 2026-11-22

## Change Summary

Remove the pistol from the run loadout, restore boss TTK with two phase gates,
replace the threat model with a wave-indexed median-build curve capped at 200
DPS, and raise the sprinter to 260 base / +30 per wave so elite sprinters
threaten a 300-speed player on average by wave 5 (guaranteed from wave 9).

## Motivation

The second cross-GDD re-review left four design concerns after the per-GDD
revision:

- the pistol was a dead one-weapon loadout (~1–17 DPS against a 300-DPS wave-30
  target);
- boss TTK fell from 22.7 s to 5.5 s across waves 5–30;
- the threat curve was still calibrated near a maxed Bullet Storm build while
  labelled a median build;
- a 300-speed player outran all non-boss melee until ~wave 16.

This spec resolves them without adding a new system.

## Design Delta

### 1. Run loadout

Current: `combat-system.md` §One weapon per run offers rifle, shotgun, and SMG
if unlocked; the pistol is also present in the Weapon Stat Reference.

New rule: the loadout offers **rifle, shotgun, and SMG if unlocked**. The
pistol is not a run option; its asset is reserved for a future Bunker sidearm
or challenge mode. The chosen weapon is locked for the run.

### 2. Boss health and phases

`boss_health = 40 + 18 × wave + 10 × floor((wave − 1)² / 12)`

| Wave | Boss HP | Expected DPS | Raw TTK | With phases | Target |
|---|---|---|---|---|---|
| 5 | 140 | 9.7 | 14.4 s | 18.4 s | 10–20 s |
| 10 | 280 | 22.3 | 12.6 s | 16.6 s | 10–20 s |
| 20 | 700 | 116 | 6.0 s | 10.0 s | 10–20 s |
| 30 | 1,280 | 200 | 6.4 s | 10.4 s | 10–20 s |

At 60% and 30% health the boss becomes **invulnerable for 2.0 s**, summons
**2 regular zombies of an unlocked type** with the standard 0.6 s spawn ring,
and emits a shockwave. After the second phase it gains **+20% speed**. Boss
scrap and score are unchanged (`25 + 5w`, `100 + 50w`). For endless waves,
`boss_health(w > 30) = boss_health(30) + 30 × (w − 30)`.

### 3. Wave-indexed expected DPS

`expected_dps(wave) = min(200, 5 × 1.18^(wave − 1))`

Reference build: one rifle, median accumulated picks — 4 Hollow Point, 4 Rapid
Fire by wave 20 (5 by wave 30), 2 Weapon Tuning, 1 Bullet Storm by wave 20 and
2 by wave 30, with the remaining drafts on build/utility. The max SMG + Bullet Storm build (~418 DPS)
is the power-trip ceiling, not the budget. Measured clear time **10–40 s** is
the gate for waves 5–30; wave 1 is the onboarding exception.

| Wave | Count | Base HP | Effective HP | Expected DPS | Target | Clear |
|---|---|---|---|---|---|---|
| 1 | 5 | 14 | 14 | 5 | 11 | 2.8 s |
| 5 | 14 | 190 | 228 | 9.7 | 15 | 23.5 s |
| 10 | 24 | 483 | 580 | 22.3 | 20 | 26.0 s |
| 20 | 44 | 2,111 | 2,533 | 116 | 30 | 21.8 s |
| 30 | 64 | 5,867 | 7,040 | 200 | 40 | 35.2 s |

Effective HP includes the guaranteed elite and random elites (~+20%). Boss
rows add the two phase gates and their adds (wave 30 ≈ +5 s). The table uses
burst DPS; the ammo faucet and sustained-DPS model are an open SYS-11
dependency, so the 10–40 s gate is not yet signed off.

### 4. Early kiting

Sprinter base speed **260**, wave bonus **30** applied as `0.1 × 30 × wave`
(+3/wave; GDD was 230/20, build is 120/30). Elite sprinters exceed the 300
player cap on average from wave 5 (wave-5 mean ≈ 302.8; guaranteed from wave 9
at minimum variance ≈ 303.1); the boss charge
remains 320 px/s with a 0.2 s lead. The 300 px/s player cap remains.

## New Rules / Values

- Loadout: rifle, shotgun, SMG-if-unlocked; never pistol.
- Boss HP: 140 / 280 / 700 / 1,280 at waves 5 / 10 / 20 / 30.
- Boss phases: 2 gates, 2.0 s invulnerability, 2 adds, +20% speed after phase 2.
- Expected DPS cap: 200; reference build as above.
- Sprinter: 260 base, +30/wave, scale 0.85, 1 HP base.
- No change to boss scrap/score, Bullet Storm ammo cost, one-weapon lock, or
  shotgun/SMG/rifle roles.

## Affected Systems

| System | Impact | Action Required |
|---|---|---|
| Combat | Weapon roster, boss phases, boss HP | Updated GDD |
| Wave | Boss HP, phases, threat table, sprinter, AI | Updated GDD |
| Economy | Boss scrap/score unchanged; note added | Updated GDD |
| Registry | Boss HP, sprinter, three new constants | Updated |
| Concept | Three run weapons | Updated |
| Systems index | Status and summary note | Updated |

## Tuning Knob Mapping

Maps to `wave-system.md` Tuning Knobs:
- **Boss health growth**: `18/wave + regular` (range 12–24/wave + regular).
- **Boss phase gate**: 2.0 s, 2 adds (range 1.0–3.0 s, 1–3 adds).
- **Expected DPS cap**: 200 (registered as `expected_dps_cap`).
- **Sprinter base/wave bonus**: 260/30 (the stat table owns this).

## Acceptance Criteria

- [ ] The loadout screen offers rifle, shotgun, and SMG if unlocked; never the
      pistol.
- [ ] Boss HP is 140/280/700/1,280 at waves 5/10/20/30.
- [ ] Each boss has two phase gates at 60%/30% HP, each 2.0 s invulnerable with
      2 adds and a shockwave; phase 2 grants +20% speed.
- [ ] Boss TTK for the reference build lands in 10–20 s at waves 5/10/20/30.
- [ ] The threat table clears in 10–40 s (waves 5–30) with the expected-DPS cap
      200, after the ammo faucet/sustained-DPS model is authored.
- [ ] An elite sprinter exceeds 300 px/s on average by wave 5 (guaranteed from
      wave 9 at minimum variance); the regular sprinter exceeds 300 px/s on
      average around wave 18 (guaranteed wave 22).
- [ ] No regression: one-weapon lock, shotgun 0.20 ammo share, Bullet Storm
      ammo cost (shotgun pellets free), boss scrap/score, Bunker 3,610.

## GDD Update Required?

**Yes — applied 2026-11-22.**
- `combat-system.md`: Weapon Stat Reference, boss rule, implementation deltas.
- `wave-system.md`: boss health/formula/AI, threat table, sprinter, knobs, ACs.
- `economy-system.md`: boss HP/phases ownership note.
- `game-concept.md`: three run weapons.
- `design/registry/entities.yaml`: boss HP, sprinter, constants.
- `design/gdd/systems-index.md`: summary/status note.
