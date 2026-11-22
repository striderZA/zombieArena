# Systems Index: Zombie Arena

> **Status**: Draft (reverse-documented from the current implementation; per-GDD review 2026-11-22)
> **Created**: 2026-11-22
> **Last Updated**: 2026-11-22
> **Source Concept**: design/gdd/game-concept.md

## System Enumeration

| ID | System | Category | MVP? | Dependencies | Owner |
|---|---|---|---|---|---|
| SYS-01 | Input & Controls | Input | Yes | None | game-designer / gameplay-programmer |
| SYS-02 | Save & Persistence | Foundation | Yes | None | engine-programmer |
| SYS-03 | Deterministic RNG & Seeding | Foundation | Yes | None | engine-programmer |
| SYS-04 | Asset Loading & Audio | Foundation | Yes | None | technical-artist / sound-designer |
| SYS-05 | Player & Movement | Character | Yes | SYS-01, SYS-02 | game-designer / gameplay-programmer |
| SYS-06 | Arena & Cover | Level/World | Yes | SYS-03 | game-designer / systems-designer |
| SYS-07 | Weapons & Ammo | Combat | Yes | SYS-01, SYS-05 | game-designer / systems-designer |
| SYS-08 | Enemy Roster & AI | AI | Yes | SYS-03, SYS-05, SYS-06 | game-designer / ai-programmer |
| SYS-09 | Combat Resolution | Combat | Yes | SYS-05, SYS-06, SYS-07, SYS-08, SYS-10 | systems-designer / gameplay-programmer |
| SYS-10 | Wave Director | Game Flow | Yes | SYS-03, SYS-06, SYS-08, SYS-09, SYS-12 | game-designer / systems-designer |
| SYS-11 | Pickups | Item | Yes | SYS-05, SYS-06, SYS-09 | systems-designer |
| SYS-12 | Upgrade Draft | Progression | Yes | SYS-03, SYS-05, SYS-07, SYS-11, SYS-10 | game-designer / systems-designer |
| SYS-13 | Meta Progression & Bunker | Economy | No | SYS-02, SYS-05, SYS-07, SYS-09, SYS-10 | economy-designer / systems-designer |
| SYS-14 | Local Co-op | Multiplayer | No | SYS-01, SYS-05, SYS-08, SYS-15 | gameplay-programmer |
| SYS-15 | HUD & Feedback | UI | Yes | SYS-05, SYS-07, SYS-08, SYS-09, SYS-10, SYS-12 | game-designer / technical-artist |
| SYS-16 | Menus, Settings & Accessibility | UI | No | SYS-01, SYS-02, SYS-04 | game-designer / ux-designer |
| SYS-17 | Audio & Music | Audio | No | SYS-04 | sound-designer |
| SYS-18 | Automated Verification | Polish | Yes | All gameplay systems | qa-lead / lead-programmer |

## Dependency Map

### Foundation

- SYS-01 Input & Controls — no dependencies (keyboard/mouse, gamepad, rebinding).
- SYS-02 Save & Persistence — no dependencies (profile, settings, hi-score).
- SYS-03 Deterministic RNG & Seeding — no dependencies (splitmix64, `Hash2D`).
- SYS-04 Asset Loading & Audio — no dependencies (textures, font, WAVs, raudio).

### Core

- SYS-05 Player & Movement → SYS-01, SYS-02.
- SYS-06 Arena & Cover → SYS-03.
- SYS-07 Weapons & Ammo → SYS-01, SYS-05.
- SYS-08 Enemy Roster & AI → SYS-03, SYS-05, SYS-06.
- SYS-09 Combat Resolution → SYS-05, SYS-06, SYS-07, SYS-08, SYS-10.
- SYS-10 Wave Director → SYS-03, SYS-06, SYS-08, SYS-09, SYS-12.
- SYS-11 Pickups → SYS-05, SYS-06, SYS-09.

### Feature

- SYS-12 Upgrade Draft → SYS-03, SYS-05, SYS-07, SYS-11, SYS-10.
- SYS-13 Meta Progression & Bunker → SYS-02, SYS-05, SYS-07, SYS-09, SYS-10.
- SYS-14 Local Co-op → SYS-01, SYS-05, SYS-08, SYS-15.

### Presentation

- SYS-15 HUD & Feedback → SYS-05, SYS-07, SYS-08, SYS-09, SYS-10, SYS-12.
- SYS-16 Menus, Settings & Accessibility → SYS-01, SYS-02, SYS-04.
- SYS-17 Audio & Music → SYS-04.

### Polish

- SYS-18 Automated Verification → all gameplay systems.

### Cycles and Bottlenecks

- **No blocking design cycle.** Two runtime couplings are intentionally mutual:
  SYS-09 ↔ SYS-10 (combat signals the last kill; the wave director owns the
  clear/game-over transition) and SYS-10 ↔ SYS-12 (wave clear opens the draft;
  picking a card starts the next wave). Ownership of the state transition lives
  in SYS-10 in both cases, which breaks the conceptual loop. The index lists
  hard and runtime edges; soft presentation/audio edges are listed in each GDD.
- Co-op depends on HUD for player-2 presentation, while HUD depends on core
  gameplay but not co-op; that dependency is one-directional.
- **Bottleneck: SYS-09 Combat Resolution.** Weapons, enemies, player, arena, and
  pickups all feed it, and HUD/meta read from it. Any change to damage, health,
  collision, or rewards must be validated here first.
- **Secondary bottleneck: SYS-10 Wave Director.** It couples enemy composition,
  arena growth, elite assignment, and the level-up transition.

## Recommended Design Order

1. SYS-01 Input & Controls
2. SYS-02 Save & Persistence
3. SYS-03 Deterministic RNG & Seeding
4. SYS-04 Asset Loading & Audio
5. SYS-05 Player & Movement
6. SYS-06 Arena & Cover
7. SYS-07 Weapons & Ammo
8. SYS-08 Enemy Roster & AI
9. SYS-09 Combat Resolution
10. SYS-10 Wave Director
11. SYS-11 Pickups
12. SYS-12 Upgrade Draft
13. SYS-15 HUD & Feedback
14. SYS-13 Meta Progression & Bunker
15. SYS-16 Menus, Settings & Accessibility
16. SYS-14 Local Co-op
17. SYS-17 Audio & Music
18. SYS-18 Automated Verification

> Reverse-documentation note: the implementation already exists, so this order is
> the **documentation** order. The current batch starts with the systems that
> carry the most cross-system facts: combat, waves, upgrades, and economy.

## High-Risk Systems

| System | Risk / reason | Mitigation or validation |
|---|---|---|
| SYS-09 Combat Resolution | Bottleneck; damage, hit cooldown, knockback, explosions, cover, and rewards all interact | Logic tests pin the rules; smoke test exercises real combat and swept bullets |
| SYS-10 Wave Director | Couples count, composition, speed/health scaling, elites, boss cadence, and arena growth | Deterministic composition tests plus wave-5 smoke assertions |
| SYS-13 Meta Progression & Bunker | Persistent economy can trivialize the game or stall progress | Capped levels, fixed costs, profile clamp on load, deterministic tests |
| SYS-06 Arena & Cover | Large bodies can be trapped; cover alters every projectile path | 150 px inset/spacing (boss body ~132 px), spawn-clear zone, resolution tests, smoke cover assertions |
| SYS-14 Local Co-op | Requires a gamepad for player 2; camera/revive rules add edge cases | Scripted gamepad input in smoke; revive and double-down assertions |
| SYS-18 Automated Verification | Tests can drift from behavior or be weakened to pass | Constraints: tests assert behavior, not defaults; run all suites before handoff |

## Progress Tracker

| ID | System | Priority tier | GDD status | GDD path |
|---|---|---|---|---|
| SYS-01 | Input & Controls | MVP | Not Started | — |
| SYS-02 | Save & Persistence | MVP | Not Started | — |
| SYS-03 | Deterministic RNG & Seeding | MVP | Not Started | — |
| SYS-04 | Asset Loading & Audio | MVP | Not Started | — |
| SYS-05 | Player & Movement | MVP | Not Started | — |
| SYS-06 | Arena & Cover | MVP | Not Started | — |
| SYS-07 | Weapons & Ammo | MVP | Not Started | — |
| SYS-08 | Enemy Roster & AI | MVP | Not Started | — |
| SYS-09 | Combat Resolution | MVP | Needs Revision | design/gdd/combat-system.md |
| SYS-10 | Wave Director | MVP | Needs Revision | design/gdd/wave-system.md |
| SYS-11 | Pickups | MVP | Designed | design/gdd/pickups.md |
| SYS-12 | Upgrade Draft | MVP | Needs Revision | design/gdd/upgrade-system.md |
| SYS-13 | Meta Progression & Bunker | Vertical Slice | Needs Revision | design/gdd/economy-system.md |
| SYS-14 | Local Co-op | Vertical Slice | Not Started | — |
| SYS-15 | HUD & Feedback | MVP | Not Started | — |
| SYS-16 | Menus, Settings & Accessibility | Vertical Slice | Not Started | — |
| SYS-17 | Audio & Music | Vertical Slice | Not Started | — |
| SYS-18 | Automated Verification | MVP | Not Started | — |

### Progress Summary

- Total systems: 18
- MVP systems: 14
- Designed this batch: 5 (combat, waves, upgrades, economy, pickups)
- Per-GDD review 2026-11-22: Combat NEEDS REVISION; Wave MAJOR REVISION NEEDED;
  Upgrade NEEDS REVISION; Economy NEEDS REVISION. All four revised.
- Cross-GDD review (second pass) 2026-11-22: FAIL — 9 unique blockers found in
  the combined loop. Approved defaults applied. Re-review 2026-11-22 found 12
  residual doc conflicts; all 12 fixed. The balance quick-spec
  (`design/quick-specs/balance-pass-2026-11-22.md`) resolved the four design
  concerns: pistol removed from the run loadout; boss HP 40+18w+regular with
  two 2.0 s phase gates; expected-DPS cap 200 with a wave-indexed median build;
  sprinter 260/30. Balance re-review 2026-11-22: consistency clean; critical
  ammo-feasibility model open (SYS-11 Pickups); boss TTK dips waves 21–28.
  Status remains Needs Revision until the ammo model is authored.
- SYS-11 Pickups GDD complete 2026-11-22: 8/8 required sections plus
  Visual/Audio and UI; 16 testable ACs. Ammo model authored (75% salvage,
  240/40 cap, late floor, shared tokens); wave 10–40 s clear-time gate still
  unsigned pending a reload/clip or late-HP decision. Pending independent
  review.
- Remaining before MVP coverage: player, weapons, enemies, arena, HUD, input,
  save, RNG, assets, verification
