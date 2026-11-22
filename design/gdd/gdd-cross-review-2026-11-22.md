# Cross-GDD Review Report

> **Superseded** — this is the pre-revision snapshot (2026-11-22). The current
> review is `design/gdd/gdd-cross-review-2026-11-22-revised.md`. Do not read
> this file as current gate evidence; its C4 (wave 5 = 11) and 3310 scrap
> figures are historical.

Date: 2026-11-22
GDDs reviewed: 4 system GDDs + game concept + systems index + registry
Systems covered: Combat Resolution (SYS-09), Wave Director (SYS-10), Upgrade
Draft (SYS-12), Meta Progression & Bunker (SYS-13)

Method: two independent read-only review passes (cross-document consistency and
game-design holism), each cross-checked against `src/` and the test suites.

---

## Consistency Issues

### Blocking

None remaining.

### Fixed during closeout

| # | Issue | Resolution |
|---|---|---|
| C1 | Systems index omitted the mutual SYS-09↔SYS-10 and SYS-10↔SYS-12 edges and claimed "no cycles" | Index now records the mutual runtime edges and names the owning state machine; the two GDDs agree that the Wave Director owns clear/game-over |
| C2 | Upgrade Draft ↔ Meta was "Soft" in one GDD and "None" in the other | Both now say None: same stats, no interface call |
| C3 | Wave-clear edge case contradicted the exploder death-blast edge case | Rule 10 now says blast damage resolves first; if the last player dies, Game Over wins (no draft, no revive) |
| C4 | Wave GDD listed wave 5 as 13 zombies instead of 11 | List corrected: waves 1–4 = 5/7/9/11; wave 5 = 11 (10 regulars + boss) |
| C5 | Combat and wave GDDs both owned contact damage, exploder blast and boss health, with conflicting ranges | Combat now points to the Wave Director's Enemy Stat Reference as owner |
| C6 | `wave_clear_scrap` was defined with and without Scrap Magnet | Wave GDD now defines `wave_clear_scrap_base`; economy owns the awarded amount and multiplier |
| C7 | Kill-scrap ceiling was documented as 225, but the real cap is 50 (Magnet 5, no elite boss) | Combat, economy and registry now say 1–50 |
| C8 | Boss-wave count clause fired at wave 0 | Formula and registry now require `wave >= 5` |
| C9 | Registry cited documents that did not state the fact and omitted one that did | Sources/references corrected; `draft_size` now cites the wave GDD |

---

## Game Design Issues

### Blockers

| # | Issue | Status |
|---|---|---|
| F1 | Regular enemies never scaled while player DPS could reach ~52×, so pressure flattened after the draft caps | **Fixed**: regular zombies gain `+1 HP` per 4 waves (`SpawnHealth`), with logic tests; GDD formula, table, knob and acceptance criteria updated |
| F2 | Killing the last zombie with an exploder blast banked the run, then the wave-clear draft overwrote Game Over; the next run re-banked scrap | **Fixed**: `EnterLevelingUp` is guarded to `Playing`, the kill block returns if the blast ended the run, and a new smoke scenario proves Game Over wins |

### Deferred (balance decisions, not contradictions)

| # | Warning | Decision |
|---|---|---|
| F3 | Max movement speed (373 px/s) outruns every enemy and projectile until very late waves | Deferred; open balance question |
| F4 | Adrenaline 2 (8 HP/kill, both players) replaces the health pickup economy | Deferred; open balance question |
| F5 | Co-op is easier than solo; a low-health player can die to heal to 50% | Deferred; open balance question |
| F6 | Boss/elite drops are lost when that kill ends the wave | Deferred; recorded as a wave GDD edge case |
| F7 | Boss is too slow and melee-only to threaten a moving player | Deferred; open balance question |
| F8 | Cover density falls as the arena grows (6 pieces capped) | Deferred; open balance question |
| F9 | Chaser becomes faster than the sprinter from wave 12 | Deferred; open balance question |
| F10 | Shared 0.25 s damage cooldown caps horde contact DPS | Deferred; open balance question |
| F11 | SMG dominates all other weapons at max upgrades | Deferred; open balance question |
| F12 | Bunker rows are weaker per point than free draft cards; Deep Pockets rarely binds | Deferred; recorded in economy open questions |
| F13 | Scrap Magnet is the strongest first buy and a wave-30 run nearly completes the 3310 sink | Deferred; recorded in economy open questions |
| F16 | Draft pool exhausts after ~39 picks, leaving a single Repair Kit | Deferred; recorded in upgrade open questions |

### Fixed or documented

| # | Issue | Resolution |
|---|---|---|
| F14 | Wave GDD clear scrap omitted Magnet | Fixed with C6 |
| F15 | Shared textures mean two enemy pairs differ only by tint/scale | Documented as a known limitation and art open question in the concept |
| F17 | Output ranges assumed an illegal elite boss | Fixed with C7; contact max 22.5 |
| F18 | Registry source errors and MVP count (11 vs 14) | Fixed |
| F19 | Concept claimed enemy variety grows every wave and ammo pressure | Concept now says variety expands through wave 4; ammo question opened |
| F20 | Draft cards hide stacks, next values and pickup timer effects | Card requirement added; open question recorded |
| F21 | No ammo/reload HUD requirement | Added to combat UI requirements |

---

## Cross-System Scenario Issues

| Scenario | Result |
|---|---|
| A. Last zombie is an exploder that kills the last player | **Fixed** (F2); Game Over now wins over the draft |
| B. Boss + cover + spitters at wave 15 | Deferred: boss pressure (F7) and cover density (F8) |
| C. Maxed build at wave 20 | **Improved** by F1; further tuning is F3/F4/F10/F11 |
| D. Co-op with one player dead and Adrenaline | Deferred: F4/F5 |
| E. Maxed draft at wave ~39 | Deferred: F16 |
| F. Scrap Magnet rush | Deferred: F13 |

---

## GDDs Flagged for Revision

None blocking. The deferred warnings above are recorded as open questions in
`wave-system.md`, `upgrade-system.md`, `economy-system.md`, and
`game-concept.md`; they do not require a GDD rewrite before architecture.

---

## Verdict: CONCERNS

No blocking issues remain. The two blockers are fixed and covered by tests
(719 logic assertions, 107 smoke checks). The remaining warnings are balance
decisions that should be settled before or alongside architecture: speed cap,
Adrenaline sustain, co-op revive, boss threat, cover density, enemy speed
identity, damage cooldown, SMG dominance, Bunker value, Magnet pacing, and the
endgame draft pool.

### Recommended next actions

1. Run `/design-review design/gdd/[system].md` in a **fresh session** for each of
   the four GDDs (independent review; do not run it in the authoring session).
2. Decide the deferred balance warnings, starting with F3 (speed cap) and F11
   (SMG dominance), because they affect weapon and meta design.
3. Then run `/create-architecture` or `/gate-check` once the warnings have
   accepted dispositions.

## Evidence

- `ctest`: 2/2 pass — 719 logic assertions, 107 smoke checks.
- New regression: last-kill exploder death keeps Game Over and does not open the
  draft (`src/SmokeTest.cpp`).
- New scaling tests: chaser/bloater/boss spawn health by wave
  (`tests/logic_tests.cpp`).
- Registry: 7 entities, 3 items, 8 formulas, 7 constants; YAML parses.

## Files Changed During Closeout

- `design/gdd/{combat,wave,upgrade,economy}-system.md`
- `design/gdd/game-concept.md`
- `design/gdd/systems-index.md`
- `design/registry/entities.yaml`
- `src/Zombie.h`, `src/Game.cpp`, `src/SmokeTest.cpp`, `tests/logic_tests.cpp`
- `docs/raylib-port.md`
