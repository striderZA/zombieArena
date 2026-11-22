# Cross-GDD Review Report (Second Pass)

Date: 2026-11-22
GDDs reviewed: 4 system GDDs + game concept + systems index + registry + ADR-0001
Systems covered: Combat Resolution (SYS-09), Wave Director (SYS-10), Upgrade
Draft (SYS-12), Meta Progression & Bunker (SYS-13)

Method: three independent read-only passes (cross-document consistency,
game-design holism, multi-system scenario walkthrough), cross-checked against
`src/`, the registry, and ADR-0001. The prior `gdd-cross-review-2026-11-22.md`
is a pre-revision snapshot and is superseded by this report.

**Verdict at review: FAIL** — 9 unique blocking issues remained in the combined
loop despite the per-GDD revision passing.

---

## Consistency Issues

### Blocking
| # | GDDs | Contradiction | Resolution |
|---|---|---|---|
| C-B1 | Wave §Rule 13 vs §Threat budget | Rule 13 used `8×1.22^(w−1)`; the formula/table used `5×1.18^(w−1)`. | Adopted one curve (`min(300, 5×1.18^(w−1))`; superseded by the balance pass: cap 200) and a named median reference build. |
| C-B2 | Combat §Rule 10 vs §Rule 2/AC | Frame order applied projectile damage before the blast priority pass, inverting the AC. | Split projectile movement from damage acceptance; all candidates resolve in one priority pass. |

### Warnings
- C-W1 Index listed SYS-11 as an SYS-13 dependency; economy says Pickups = None.
- C-W2 Index statuses were stale relative to the revised GDDs.
- C-W3 Concept carried “100 bullets”; contract is 160.
- C-W4 Concept carried “ammo pickup value”; Deep Pockets is starting spare ammo.
- C-W5 Wave UI said keys 1–3; upgrade allows a fourth card (1–4 / A/B/X/Y).
- C-W6 `bestWave` used waves cleared, contradicting “wave reached.”
- C-W7 Victory “large score bonus” had no value.
- C-W8 Combat score range said 10–300; real regular max is 45.
- C-W9 Wave-clear example omitted the depth multiplier.
- C-W10 Pacing prose said a wave-2 death takes 30–60 runs; the table implied ~100.
- C-W11 Cover formula said 0 at wave 0; the edge case treats wave 0 as wave 1.
- C-W12 Endless scrap ranges exceeded the registry’s finite caps.
- C-W13 Fallback + low-health fourth card could duplicate Repair.
- C-W14 Spawn inactivity was not gated in combat’s sweep/contact interface.
- C-W15 First Bunker purchase was unearnable on a true wave-1 death.
- C-W16 Upgrade AC said “exactly three” without the fourth-card caveat.

---

## Game Design Issues

### Blocking
| # | GDDs | Problem | Resolution applied |
|---|---|---|---|
| D-B1 | Wave | Threat budget validated against an armchair curve, not a build. | Median one-weapon reference build named; one curve. |
| D-B2 | Wave + Upgrade + Combat | 420 DPS cap was maxed SMG + Bullet Storm; every other build cleared wave 30 in 55–68 s, outside the band. | Bullet Storm costs 1 extra round per projectile; HP growth lowered to `(w−1)²/12`; threat cap 300 (superseded by the balance pass: 200); target band 10–40 s. |
| D-B3 | Combat + Economy | SMG strictly dominated the rifle (DPS, reserve, spread). | SMG sidegrade: 30/60/8.0/7°, 0.5 ammo share; rifle keeps precision/sustain. |
| D-B4 | Combat + Wave | Max speed 373 outran every in-run melee; body-block unspecified. | Speed capped at 300; boss charge 320 with lead; elite sprinter/chaser exceed 300; body-block explicitly off (contact is the cost). |
| D-B5 | Economy | True wave-1 death banks 0–5 vs 10 cheapest row. | One-time 10-scrap first-run stipend; dead-player co-op case excluded. |
| D-B6 | Upgrade | Build subset had 12 stacks; a 30-draft run exhausted it by ~wave 13. | Build cards deepened to 28 total stacks (max 5 except Bullet Storm 2, Ricochet 3, Explosive 3) and two build cards guaranteed per draft; the guarantee lasts ~14 drafts, so later drafts are stat/Repair drafts. |
| D-B7 | Upgrade + Combat | Loadout lock/switch rule undefined. | Run locks to one weapon; co-op players choose independently. |
| D-B8 | Wave + ADR + Combat | 132 px boss body vs 90 px gaps / 100 px wall inset; boss could be walled off. | Cover inset/spacing 150 px; ADR guarantee corrected; boss-through-gap AC added. |
| D-B9 | Wave | Spitter 150–250 band and no lead created no positioning pressure. | Band 350–500, range 600, lead 0.3 s; boss charge leads. |

### Warnings
- D-W1 Concept promised power-trip at wave 10; design schedule is wave 21. Concept now says wave twenty.
- D-W2 Victory bonus undefined. Now 5,000.
- D-W3 Boss HP linear vs regular quadratic. Boss HP now includes the regular-scaling term.
- D-W4 Endless had no contract. Now: regular HP frozen at wave 30, count capped, elite 25%, boss HP +30/wave.
- D-W5 “Score safe from Bunker” was ambiguous. Now stated as a progression score; no-Bunker board out of scope.
- D-W6 Attention budget. Absorb cue throttled to 0.5 s; damage numbers aggregate per 0.35 s and cap at 12; ammo HUD simplified to one weapon.
- D-W7 Silhouette/elite gold. Elite accent is now a white rim/crown; sprinter gets a motion trail/tell; distinct silhouettes remain an art task.
- D-W8 Spitter positioning. Fixed with the 350–500 band + lead.
- D-W9 Co-op easier than solo. Surcharge now `3 + floor(1.5w)`.
- D-W10 Bunker choice solved/Pockets dominated. Veteran Badge capstone added; Pockets remains a balance-pass item.
- D-W11 “Days/Weeks” ~1 hour. Concept now says “first few sessions.”
- D-W12 Threat table excluded elite doubling. Table now has Base HP and Effective HP columns.
- D-W13 Stowed-reload rules. Moot: one weapon per run; reload cancelled by damage/death.
- D-W14 Loadout state missing. Wave state table now has `Loadout`.
- D-W15 Player–zombie body-block. Explicit: no body-block; passing through costs contact.
- D-W16 Ammo pickup effective value. One weapon per run; pickup is 16 (SMG 8 via ammo share).

---

## Cross-System Scenario Issues

Scenarios walked: 6 — last-kill exploder fuse + dead player; boss/elite last kill → drops/draft/reset; post-bank bullet kill/second Game Over; wave-30 Victory → endless; co-op shared draft/purchase/revive; boss chase vs max-arena cover.

### Blockers
- **S6 boss/cover geometry** — 132 px boss vs 90/100 px geometry. Fixed with 150 px inset/spacing.
- **S1/B1 first Bunker purchase** — true wave-1 death. Fixed with the first-run stipend.

### Warnings
- S1: the fuse delta needed an explicit “defer EnterLevelingUp until fuses resolve.” Added to the combat delta.
- S2: boss/elite drops spawn before the clear but are reset by the next wave; the concept’s “guaranteed drops” promise is still a known limitation.
- S3: post-bank guard is now all-reward and all-transition; projectile death wins over a later last-kill.
- S4: Victory bonus/order now defined; “past 30” appears once; depth capped 1.9; post-cap split defined.
- S5: dead players no longer trigger the low-health card; Repair uniqueness and Bunker input precedence clarified.

### Info
- Second Game Over is unreachable (defensive clause).
- Two same-frame blasts need a cue tie-break.

---

## GDDs Flagged for Revision

| GDD | Reason | Type | Priority | Status after review |
|---|---|---|---|---|
| wave-system.md | C-B1, D-B1/B2/B4/B8/B9, C-W5/C-W7/C-W9/C-W11/C-W12/C-W14 | Consistency + design | Blocking | Revised |
| combat-system.md | C-B2, D-B2/B3/B4, C-W8/C-W14, D-W13/D-W15/D-W16 | Consistency + design | Blocking | Revised |
| upgrade-system.md | D-B2/B6/B7, C-W13/C-W16, D-W13 | Design | Blocking | Revised |
| economy-system.md | D-B3/B5, C-W6/C-W7/C-W9/C-W10/C-W12, D-W5/D-W10/D-W11 | Design + consistency | Blocking | Revised |
| game-concept.md | C-W3/C-W4, D-W1/D-W5/D-W11 | Consistency | Warning | Revised |
| systems-index.md | C-W1/C-W2 | Consistency | Warning | Revised |
| ADR-0001 | D-B8, verification date | Consistency | Blocking | Revised |

---

## Post-Review Resolution

The approved defaults were applied to the GDDs, registry, concept, index, and
ADR:

- one threat-budget curve and a named median reference build;
- priority-pass damage resolution;
- Bullet Storm 1-extra-round cost; SMG sidegrade; speed cap 300; boss charge
  lead; body-block rule;
- one-time 10-scrap first-run stipend;
- two build cards per draft and deeper build stacks (28 total; guarantee lasts
  ~14 drafts);
- one-weapon loadout lock;
- cover inset/spacing 150 px for the 132 px boss;
- spitter 350–500 band, 600 range, 0.3 s lead;
- co-op surcharge `3 + floor(1.5w)`;
- depth cap 1.9 and post-cap split;
- Veteran Badge capstone (total 3,610);
- all stale concept/index/registry/ADR values.

## Verdict: FAIL at review; blockers addressed, re-review required

The GDDs now encode the approved fixes, but the skill requires a fresh
`/review-all-gdds` (or `/consistency-check` + targeted design pass) before
architecture. Residual balance warnings remain: Pockets value, boss threat at
max arena, endless contract depth, distinct silhouettes, and co-op draft agency.

---

## Re-Review — 2026-11-22

**Consistency: 12 conflicts found, 12 fixed.** The mechanical fixes landed:
concept `/12` and revised-design header; index 150 px and status note; endless
boss formula (`boss_health(30) + 30×(w−30)`); threat table replaced Budget with
Clear s; effective-HP label/elite note; Adrenaline 5 stacks; ADR white elite
rim and verification wording; wave Victory state order; concept guaranteed-drop
caveat; report 28 build stacks; registry duplicate notes merged and revised
dates set; SMG/Scavenger ammo order; build-guarantee durability documented.

**Design: RESIDUAL CONCERNS (4).**
- Pistol is a dead loadout under the one-weapon lock (~1–17 DPS vs a 300-DPS
  wave-30 target). Remove it from the loadout, rebuff it, or label it a
  challenge option.
- Boss TTK falls 22.7 s → 5.5 s across waves 5–30; the regular-scaling term did
  not restore the climax. Add phases/damage gates or hold TTK ~10–15 s.
- The threat model is still calibrated near a maxed Bullet Storm build despite
  the "median reference build" label. Re-derive expected DPS from a
  wave-indexed accumulated build before architecture.
- A 300-speed build still outruns non-boss melee until ~wave 16; validate
  kiting against a 300-speed bot.

Fixed during re-review: spitter lead now uses projectile flight time (not a
fixed 0.3 s); shotgun ammo share is 0.20 (3 shells/pickup); Bullet Storm pellets
are free on the shotgun (extra rounds only for single-projectile weapons).

**Verification:** `ctest` 2/2; OCGS validation 92/92; registry YAML parses with
7 entities, 3 items, 9 formulas, 16 constants.

**Gate:** not ready for `/create-architecture` until the four residual design
concerns have an accepted disposition.

---

## Balance Pass Resolution — 2026-11-22

The quick-spec `design/quick-specs/balance-pass-2026-11-22.md` resolved the
four residual design concerns:

- **Pistol:** removed from the run loadout (rifle, shotgun, SMG-if-unlocked).
- **Boss TTK:** HP `40 + 18w + 10×floor((w−1)²/12)` plus two 2.0 s phase gates
  with 2 adds; TTK 18.4/16.6/10.0/10.4 s at waves 5/10/20/30.
- **Threat model:** expected DPS `min(200, 5×1.18^(w−1))` against a
  wave-indexed median rifle build; clear times 23.5/26.0/21.8/35.2 s.
- **Early kiting:** sprinter 260/30; elite sprinters exceed the 300 cap on
  average from wave 5 (guaranteed from wave 9 at minimum variance); boss
  charge remains 320 with lead. Balance re-check found the 10–40 s gate is
  computed on burst DPS and ammo is not yet modeled; this remains open.

A final re-review is required to confirm the GDD/registry edits.

---

## Balance Re-Review — 2026-11-22

**Consistency:** 17 findings. Fixed: upgrade loadout still offered the pistol;
sprinter +3/wave wording and variance assumptions; phase invulnerability now
lasts until both adds die or 2.0 s (with immune cue); +20% applies to base chase
only; shotgun Bullet Storm costs 1 shell; clear-time gate scoped to waves 5–30;
reference build corrected to 5 Rapid Fire at wave 30 (200 DPS); boss speed delta;
economy tuning table split; registry counts (16 constants).

**Critical open — ammo feasibility.** The 10–40 s table uses burst DPS. Reload
plus the current 12 s pickup cycle gives the median rifle ~100 sustained DPS,
and wave-30 ammo is short (~43–90% of HP depending on Scavenger/elite drops).
The gate is explicitly marked unsigned until SYS-11 Pickups authors the ammo
faucet (per-kill drops or wave resupply) or late-wave HP is reduced.

**High open:** boss clear times add ~4 s of phase dead time + adds (wave 30
≈40.7 s); boss TTK dips to ~8.5 s around waves 21–28; the reference build is
only enumerated at waves 20/30.

**Medium open:** SMG/shotgun roles are not validated on the threat table;
sprinter HP is not actually fragile at high waves; phase adds are only a
pressure source if ignored.

**Verdict:** consistency clean; design **RESIDUAL CRITICAL** until the ammo
model is authored. Not ready for architecture.
