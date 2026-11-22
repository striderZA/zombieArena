# Combat System

> **Status**: In Review — revised design 2026-11-22; implementation deltas pending
> **Author**: systems-designer / gameplay-programmer (documented from code)
> **Last Updated**: 2026-11-22
> **Last Verified**: source inspection 2026-11-22 (suites not executed)
> **Implements Pillar**: Escalating, readable pressure

> **Source of truth**: `src/Game.cpp` (`UpdateCollisions`, `ExplodeAt`,
> `UpdateEnemyShots`), `src/Bullet.cpp`, `src/SweptCollision.h`,
> `src/Player.cpp`, `src/Zombie.cpp`. This GDD describes the **revised
> contract**; where the build differs, the delta is listed under
> "Implementation Deltas". ADR-0001 covers the cover collision approach.

## Overview

Combat resolution converts player shots, zombie contact, spitter projectiles,
and exploder deaths into health changes, score, and scrap. It owns the damage
cooldown, knockback, kill rewards, and the ordering rules that decide what
happens when several hits land in the same frame.

## Player Fantasy

The player should feel that every hit is earned and every hit landed is
readable: a wall of lead visibly shreds a bloater, a spitter's green shot is a
telegraphed threat, and an exploder's fuse is a mistake the player can escape.
Combat is tense but never opaque — when the player dies, they can name the
mistake. **Silence is never used to represent a hit.** A damaged hit, an
absorbed hit, and a missed hit must each read differently.

## Detailed Design

### Core Rules

1. **Damage sources.** Player bullets, zombie contact, spitter projectiles,
   exploder blasts, and ricochet/explosive-round secondary effects are the only
   damage sources. Environmental/cover damage does not exist.
2. **Shared damage cooldown with outcome truth.** A player can take damage at
   most once per `0.25 s`, across every source. A hit inside the window is
   **absorbed**:
   - **Damaged** → health loss plus the full feedback package (sound, shake,
     vignette, damage number).
   - **Absorbed** → no health loss, but a distinct short shield flash and soft
     tick; a projectile is consumed with an absorb spark; an explosion shows an
     absorb ring. No full hit sound/shake.
   - **Dodged** → no effect.
   When several sources land in the same frame, resolve in priority order:
   **exploder blast > spitter projectile > contact**. The highest-priority
   source is accepted first; lower-priority sources are absorbed with their
   cue. A low-damage tick can never silently nullify a blast.
3. **Death threshold.** Zombies die when their health becomes **zero or
   less**. Listed health is effective health: `hits_to_kill =
   ceil(health / damage)`. A 1-health chaser dies to one 1-damage hit. This
   replaces the original SFML `< 0` behavior; see Implementation Deltas.

   Player baseline: **100 max health**, **200 px/s base move speed in every
   direction** (input is normalized), capped at **300 px/s** after upgrades,
   and the **0.25 s** hit cooldown above. Meta and draft upgrades modify these.
4. **Bullet sweep and pool.** Bullets test the full segment from their previous
   position to their current position, **including the segment that exhausts
   range**, against zombies and cover. The nearest impact wins along that
   segment; at an equal distance, cover wins. Inactive spawns are not valid
   targets. A bullet can hit only one **direct** target; piercing, ricochet, and explosive cards add secondary effects from
   the upgrade GDD. The pool holds **160** bullets; a shot is never allowed to
   overwrite a live bullet. If the pool is somehow exhausted, the shot is
   dropped and counted, not silently overwritten. A shot consumes **1 round
   plus 1 round per extra Bullet Storm projectile**.
5. **Cover blocks damage.** A bullet or spitter projectile that reaches cover
   first is consumed; it cannot damage a zombie or player behind it. Explosions
   ignore cover (see rule 10).
6. **Knockback.** Bullet hits push zombies by `5 + 2 × damage` pixels, scaled by
   `1 / scale²`. Knockback applies **once per zombie per frame**, using the
   highest-damage pellet of a shotgun blast. Lethal hits apply no knockback and
   the corpse is not resolved out of cover. Bosses are large and therefore
   resist most knockback.
7. **Kill rewards.** A kill awards score, scrap (banked at `FinishRun`; the
   economy applies the Magnet/depth multipliers once), one kill counter, and
   any per-kill healing. Exploders
   start a **0.35 s fuse**; bosses and elites drop pickups; bosses also trigger
   a large **visual-only** shockwave.
8. **Elites modify, not replace.** An elite keeps its type's behavior and
   doubles health, multiplies speed by 1.15, scale by 1.25, contact damage by
   1.5, and score/scrap by 3. Elite tint multiplies the type tint; it never
   replaces it.
9. **Boss cadence.** Every fifth wave includes one boss. Boss health is
   `40 + 18 × wave + 10 × floor((wave − 1)² / 12)`; it uses the bloater sprite
   at 2.2 scale and has a HUD bar. At 60% and 30% health the boss becomes
   **invulnerable until both phase adds die or 2.0 s elapse, whichever is
   longer**, summons 2 regular zombies of an unlocked type with the standard
   0.6 s spawn ring, and emits a shockwave; the boss bar marks the 60%/30%
   gates and the boss shows an immune ring + floating "IMMUNE" cue. After the
   second phase it gains +20% **base chase** speed (the 320 px/s charge is
   unchanged). Phase gates resolve **sequentially**: a single hit cannot skip a
   gate, and excess damage is capped at the gate threshold until the phase
   ends. Target boss TTK is 10–20 s for the expected build.
10. **Resolution order and run-bank integrity.** Frame order is: player
    input/fire → zombie move and spitter fire → projectile movement → bullet
    movement → **candidate-hit collection** → damage priority pass (exploder
    blast > spitter projectile > contact) → pickups. Movement never applies
    damage; only the priority pass does, so a low-priority hit cannot consume
    the window before a blast. If the last zombie dies, contact damage and pickups are skipped for
    that frame; the wave clears after any active exploder fuses resolve. If the
    blast also kills the last player, Game Over wins: no draft opens and no
    revive occurs. **Once `FinishRun` banks the run, no kill awards score,
    scrap, or kills, no wave-clear score/scrap or `ForceSpawn` occurs, and no
    further Game Over transition occurs.** Any death of the last player in the
    frame wins over the wave clear, regardless of source.
11. **One weapon per run.** The loadout offers rifle, shotgun, and SMG if
    unlocked; the pistol is not a run option. The chosen weapon is locked for
    the run. There is no weapon cycling. Co-op players choose independently.
    Ammo pickups apply only to the equipped weapon.

### States and Transitions

| Entity | State | Transition | Trigger |
|---|---|---|---|
| Player | Alive | → Dead | Health reaches 0 or below |
| Player | Dead | → Alive | Co-op revive at the next wave (50% health) |
| Player | Any | → Game Over | All players dead |
| Player | Alive | Absorbing | A hit is rejected by the damage window; shows absorb cue |
| Zombie | Alive | → Dead | `health <= 0` after damage |
| Zombie | Dead | (terminal) | Corpses remain drawn for the wave only |
| Exploder | Dead | Fuse | 0.35 s, then blast resolves |
| Bullet | In flight | → Stopped | Hit, cover, or range exhausted **after** its final segment resolves |
| Run | Live | → Banked | `FinishRun`; no further rewards or transitions |

### Interactions with Other Systems

| System | Direction | Interface |
|---|---|---|
| Player & Movement | Reads | `Rect()`, `Center()`, `TakeHit()`, `Heal()`, `Alive()` |
| Weapons & Ammo | Reads | Bullet damage, direction, previous/current position, reload time |
| Enemy Roster & AI | Reads/writes | `Active()`, `Hit()`, `Knockback()`, `ResolveObstacles()`, `Elite()`, `Type()` |
| Arena & Cover | Reads | `Obstacles()` rectangles for segment and projectile blocking |
| Wave Director | Writes | Signals wave clear when the last zombie and active fuses resolve; the Wave Director owns the transition |
| Pickups | Writes | Boss/elite `ForceSpawn()`; player `Heal()`/ammo add on pickup |
| HUD & Feedback | Writes | Damage/absorb feedback, health, ammo, reload, damage numbers |
| Meta Progression | Writes | Score, scrap award, kill counter, hi-score |
| Save & Persistence | Writes | Hi-score saved on shutdown; profile saved at run end |
| Local Co-op | Reads | Player count, per-player cooldown and health |
| Audio & Music | Writes | Shot, hit, absorb, reload, explosion, death cues |

## Formulas

### Movement speed

`move_speed = min(300, 200 × 1.03 ^ shoes_level × 1.10 ^ swift_stacks)`

**Variables:** shoes 0–5, Swift 0–5. **Output Range:** 200–300 px/s in every
direction (the cap binds at high upgrade totals). Input is normalized; a
keyboard diagonal is no longer faster than a gamepad diagonal. **Example:**
Shoes 5 + Swift 5 = `200 × 1.1593 × 1.6105 = 373.4`, capped to **300**.

### Weapon Stat Reference (provisional; SYS-07 owns)

Each player carries **one weapon for the run**, chosen at the loadout screen.
The other weapons are not carried, and ammo pickups apply only to the equipped
weapon.

| Weapon | Clip | Starting spare | Fire rate | Spread | Pellets | Ammo share | Role |
|---|---|---|---|---|---|---|---|
| Shotgun | 2 | 12 | 0.8 | 20° | 6 | 0.20 | Burst / knockback |
| Rifle | 20 | 90 | 4.0 | 5° | 1 | 1.0 | Sustained precision |
| SMG | 30 | 60 | 8.0 | 7° | 1 | 0.5 | High-DPS burst, ammo-hungry |

An ammo pickup grants `round((16 + 8 × Scavenger) × ammo_share)` rounds/shells to
the equipped weapon (SMG at Scavenger 0: 8; shotgun: 3 shells).
The SMG is a sidegrade: highest burst DPS, a smaller reserve than the rifle,
and a tighter spread than the shotgun, with half pickup share. The rifle is the
sustained/precision option; the shotgun is burst/knockback. The loadout offers
rifle, shotgun, and SMG if unlocked; the pistol asset is reserved for a future
Bunker sidearm.

### Bullet damage

The bullet_damage formula is defined as:

`bullet_damage = 1 + hollow_point_stacks + weapon_tuning_level`

**Variables:**

| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| Base damage | 1 | int | 1 | Rifle/SMG/shotgun pellet base |
| Hollow Point stacks | `hollow_point_stacks` | int | 0–4 | +1 damage per stack |
| Weapon Tuning level | `weapon_tuning_level` | int | 0–2 | Bunker upgrade, +1 per level |

**Output Range:** 1 to 7 per projectile under current caps.
**Example:** 2 Hollow Point stacks + 1 Weapon Tuning = 4 damage per pellet.

### Fire rate

`effective_fire_rate = weapon.fire_rate × 1.2 ^ rapid_fire_stacks`

**Variables:** weapon base 0.8–8.0 shots/s; Rapid Fire 0–5. **Output Range:**
0.8 to ~19.9 shots/s (SMG at 5 stacks), frame-quantized at 60 FPS; the rate is a
parameter, not a guaranteed achieved count. **Example:** rifle with 3 stacks =
`4.0 × 1.728 = 6.912 shots/s`.

### Projectiles per shot

`projectiles = max(1, weapon.pellets + bullet_storm_stacks + (weapon == shotgun ? shotgun_choke_stacks : 0))`
`ammo_per_shot = 1 + (weapon == shotgun ? 0 : bullet_storm_stacks)`

**Variables:** pellets 1–6; Bullet Storm 0–2; Shotgun Choke 0–3.
**Output Range:** 1 to 11 pellets; 1–3 rounds per shot. Each extra Bullet Storm
projectile costs one additional round, so a 3-projectile shot consumes 3 rounds.
**Example:** shotgun with 2 Choke and 1 Storm = `6 + 1 + 2 = 9` pellets for 2 shells.

### Clip size and reload

`effective_clip = max(1, round(weapon.clip × (1 + 0.5 × extended_mag_stacks)))`

`reload_time = weapon.reload_time / (1 + 0.10 × scavenger_stacks)`

| Weapon | Base reload |
|---|---|
| Shotgun | 0.80 s |
| Rifle | 0.60 s |
| SMG | 0.55 s |

**Variables:** Extended Mag 0–5; Scavenger 0–3. **Output Range:** clip 2–105;
reload 0.39–0.80 s. An empty clip plays a dry-fire click; reloading shows a
progress indicator. Because the run carries one weapon, there is no stowed
reload: the reload is cancelled only by taking damage or dying.
**Example:** SMG with 2 Extended Mag = `round(30 × 2.0) = 60`; Scavenger 3
reduces its reload to `0.55 / 1.3 = 0.423 s`.

### Contact and projectile damage

`contact_damage = base_contact_damage(type) × (elite ? 1.5 : 1)`

**Variables:** base 10 default, 15 exploder, 20 boss; elites ×1.5; bosses never
elite. **Output Range:** 10–22.5 per accepted hit. **Spitter projectile:** fixed
`10` damage, `280 px/s`, `7 px` radius, `4 s` life, blocked by cover.

### Knockback

`knockback_distance = (5 + 2 × bullet_damage) / scale²`

**Variables:** damage 1–7; scale 0.85–2.2, elite ×1.25. Applied once per zombie
per frame; lethal hits apply zero. **Output Range:** ~1.4 px (boss, 1 damage) to
~26.3 px (sprinter, 7 damage). **Example:** 3-damage hit on a chaser = 11 px.

### Exploder fuse and blast

`fuse = 0.35 s` after death, then:
`distance(explosion, player) ≤ 110 → 20 damage; otherwise 0`

**Variables:** radius 110 px, hard edge, no falloff; 20 damage, subject to the
0.25 s cooldown and blast priority. The blast damages **players only**; it does
not chain, damage zombies, or scale with elite. **Output Range:** 0 or 20.
**Example:** a player 109 px away with no accepted hit in the last 0.25 s takes
20; at 111 px, 0.

### Kill rewards

`score += score_value(type) × (elite ? 3 : 1) + (boss ? 50 × wave : 0)`
`scrap += base_scrap(type, wave) × (elite ? 3 : 1)`

`base_scrap(type, wave) = scrap_value(type)`; boss: `25 + 5 × wave`.

**Variables:** score 10/15/100; scrap 1/2/25 (boss adds 5×wave); elite ×3. The
Scrap Magnet and depth multipliers are applied once by the economy at
`FinishRun`, not per kill. **Output Range:** regular score 10–45 per kill;
boss score `100 + 50 × wave`; scrap 1–175 at wave 30 before the bank multiplier
(unbounded in endless).
**Example:** elite spitter at wave 10 = score 45, scrap `2 × 3 = 6` before the
bank.

### Bullet pool ceiling

`in_flight ≈ projectiles_per_shot × effective_fire_rate × 1.0 s`

**Output Range:** 59.7 in flight per maxed SMG player; 119.4 for two players.
The pool is 160, so no live bullet is overwritten. **Example:** SMG, Rapid Fire
5, Bullet Storm 2 = `3 × 19.9 × 1.0 = 59.7`.

## Edge Cases

- **If a hit is absorbed**: no health loss; show the absorb cue. A consumed
  spitter projectile leaves an absorb spark. A nullified blast shows an absorb
  ring but no player-hit sound/shake.
- **If a blast and a projectile land in the same frame**: the blast resolves
  first; the projectile is absorbed if the blast was accepted.
- **If two players die in the same frame**: enter Game Over once; the co-op
  revive path does not run after Game Over.
- **If the last zombie dies while a zombie is touching the player**: bullets
  resolve first, so the wave clears and contact damage is skipped that frame.
- **If two bullets reach the same zombie in one frame**: the first processes the
  hit; if it kills, the second skips the dead zombie and may hit behind it.
- **If cover and a zombie are at the same segment distance**: cover wins.
- **If a bullet starts at the player centre** (not the muzzle): it cannot start
  inside cover because the player's centre is kept 20 px clear; the muzzle
  flash is purely visual.
- **If a spitter dies with a projectile in flight**: the projectile continues
  until it hits, expires, or leaves the arena buffer.
- **If an exploder dies**: kill rewards resolve immediately, then a 0.35 s fuse
  starts. The wave does not clear until the fuse resolves. If the blast kills
  the last player, Game Over wins.
- **If a boss or elite is knocked back into cover**: it is pushed out
  immediately after knockback; a lethal hit leaves the corpse where it died.
- **If the SMG is locked**: it is not offered in the loadout screen.
- **If a player runs through a zombie**: zombies do not body-block the player;
  passing through costs a contact hit subject to the 0.25 s cooldown.
- **If a spawn is still inactive**: bullets and contact pass through it, it
  deals no damage, and it awards no kill rewards until its 0.6 s ring expires.
- **If a player is dead**: they take no damage, cannot fire, and are skipped by
  pickups and zombie targeting.
- **If a kill lands after the run is banked**: it awards no score, scrap, or
  kills and cannot trigger another Game Over.

## Dependencies

| System | Type | Interface |
|---|---|---|
| Player & Movement | Hard | Health, normalized movement, hit cooldown, revive, rect/centre, heal |
| Weapons & Ammo | Hard | Damage, fire rate, spread, projectiles, clip, reload |
| Enemy Roster & AI | Hard | Active/spawn gate, health, type, elite flag, contact damage, knockback, fuse |
| Arena & Cover | Hard | Obstacle rectangles for segment/projectile blocking |
| Wave Director | Hard | Zombie population, active fuses, wave-clear transition |
| Pickups | Soft | Boss/elite drops, heal/ammo restore |
| Meta Progression | Soft | Scrap bank multiplier, weapon tuning |
| Save & Persistence | Soft | Hi-score and profile persistence |
| HUD & Feedback | Soft | Damage/absorb feedback, shake, blood, vignette |
| Local Co-op | Soft | Player count, per-player cooldown, revive |
| Audio & Music | Soft | Shot, hit, absorb, reload, explosion, death cues |

## Tuning Knobs

| Knob | Current | Safe Range | If Too High | If Too Low |
|---|---|---|---|---|
| Player hit cooldown | 0.25 s | 0.10–0.50 s | Player becomes invincible while kiting | Hordes delete the player instantly |
| Absorb cue duration | 0.15 s, throttled to 0.5 s | 0.08–0.25 s | Visual noise | Player cannot tell a hit was absorbed |
| Bullet speed | 1000 px/s | 600–1600 | Sweep still catches hits; visuals stretch | Leading targets becomes mandatory |
| Bullet range | 1000 px | 600–1400 | Cross-arena sniping trivializes spitters | Forced close range against bloater |
| Base bullet damage | 1 | 1–3 | Every weapon one-shots chasers at 2+ | Bloater becomes a bullet sponge |
| Knockback | `5 + 2×damage` | 0–2× current | Kiting replaces cover | Melee pressure becomes unmanageable |
| Enemy shot speed | 280 px/s | 200–450 | Projectiles become undodgeable | Spitters stop mattering |
| Reload time | 0.50–0.80 s | 0.30–1.20 s | Weapons feel unresponsive | Reload discipline disappears |
| Exploder fuse | 0.35 s | 0.20–0.50 s | Free escape | Player cannot react |
| Bullet pool | 160 | ≥ 120 | Memory cost | Co-op shots vanish |

Enemy stat knobs (contact damage, exploder blast, boss health) are owned by the
Enemy Stat Reference in `wave-system.md`; change them there first.

## Visual/Audio Requirements

- Muzzle flash: short line + orange/yellow circles at the barrel.
- Hit flash: zombie tint briefly shifts to `{255,120,120}`.
- Absorb cue: player shield flicker + soft tick, throttled to once per 0.5 s;
  projectile absorb spark.
- Blood: short-lived decals (4.5 s hit / 9 s kill), cleared per wave; not
  "persistent".
- Damage numbers: one aggregate number per target per 0.35 s, capped to the 12
  nearest targets; white→red by value.
- Screen shake: once per shot/frame, trauma scaled per event.
- Damage vignette: a true edge vignette (four ~70 px edge bars), alpha ≤ 0.18,
  honoring a `vignette` settings toggle.
- Spitter projectile: green orb with a dark outline; a charge/fire sound is
  mandatory.
- Explosion: orange shockwave, radius equal to the damage radius; the exploder
  pulses during its 0.35 s fuse.
- Audio: shoot, splat, hit, absorb, reload, dry-fire, pickup, power-up, death
  scream, music.
- Boss: red aura ring and health bar; elite: a white pulsing rim/crown that
  **multiplies** the type tint. Gold stays reserved for scrap/reward.

## UI Requirements

- Player health bar: fixed track, `health / maxHealth` fill, current/max not
  numeric.
- Boss health bar when a boss is alive.
- Ammo HUD: the equipped weapon, clip/spare, a low-ammo icon, a reload progress
  indicator, and a red clip warning on empty; co-op shows player 2's ammo
  separately.
- Damage numbers in world space; absorb cue near the player.
- Damage vignette and screen shake honor the settings toggles.
- Hi-score and score update immediately on kills.

## Acceptance Criteria

- **GIVEN** a 1-health chaser, **WHEN** it takes one 1-damage bullet, **THEN**
  it dies.
- **GIVEN** a player was just hit, **WHEN** another source hits within 0.25 s,
  **THEN** health is unchanged and the absorb cue fires, not the full hit cue.
- **GIVEN** a 109 px exploder blast and no accepted hit in the previous 0.25 s,
  **WHEN** it resolves, **THEN** the player takes 20; at 111 px, zero.
- **GIVEN** a spitter projectile and an exploder blast land on the same frame,
  **WHEN** the frame resolves, **THEN** the blast is accepted first and the
  projectile is absorbed.
- **GIVEN** a bullet whose final range-exhausting segment crosses a zombie,
  **WHEN** the segment resolves, **THEN** the zombie is hit.
- **GIVEN** cover between a bullet and a zombie, **WHEN** the bullet sweeps,
  **THEN** cover is hit first and the zombie takes no damage.
- **GIVEN** an elite spitter at wave 10, **WHEN** it is killed, **THEN** score
  +45, scrap +6 before the bank multiplier, and a pickup is forced.
- **GIVEN** the last zombie dies while touching a player, **WHEN** the frame
  resolves, **THEN** the level-up screen opens and the player takes no contact
  damage that frame.
- **GIVEN** two players at 0 health, **WHEN** the frame resolves, **THEN** Game
  Over occurs exactly once.
- **GIVEN** a boss with `scale 2.2`, **WHEN** hit for 4 damage, **THEN**
  knockback is 2.69 px; a sprinter hit for 7 damage is pushed 26.30 px.
- **GIVEN** a locked SMG, **WHEN** the loadout screen opens, **THEN** the SMG is
  not offered.
- **GIVEN** a loadout choice, **WHEN** the run starts, **THEN** only the chosen
  weapon is equipped, the pistol is not offered, and ammo pickups apply only to
  it.
- **GIVEN** an empty clip, **WHEN** the player reloads, **THEN** the reload
  takes the weapon's reload time and the HUD shows progress.
- **GIVEN** two players at max Rapid Fire/Bullet Storm, **WHEN** both fire for
  5 s, **THEN** no in-flight bullet is destroyed by the pool.
- **GIVEN** a run is banked, **WHEN** a bullet kills a zombie afterwards,
  **THEN** no score, scrap, or kill is awarded and no second Game Over occurs.

Evidence: `tests/logic_tests.cpp` (`TestZombie`, `TestSpecialZombies`,
`TestEliteZombie`, `TestBullet`, `TestSweptCollision`, `TestPlayer`,
`TestPlayerHealth`) and the smoke checks for combat, boss, co-op, and cover.
New tests are required for the revised death threshold, absorb cue, terminal
sweep, reload duration, pool ceiling, and post-bank guard.

## Implementation Deltas

The GDD is the contract; these deltas are tracked for the implementation pass:
- Death threshold is `< 0` in `src/Zombie.cpp`; revise to `<= 0` and update tests.
- Movement input is unnormalized per axis; normalize in `src/main.cpp`/`Player`.
- Bullets stop before the terminal segment resolves; reorder in `Game.cpp`.
- The bullet pool is 100; raise to 160.
- Weapon cycling exists in the build; replace it with the one-weapon loadout
  and remove the pistol from the run options.
- Boss HP is `40 + 18w + 10×floor((w−1)²/12)` and gains two 2.0 s phase gates
  with 2 adds each.
- SMG stats are 30/120/8.0/4°; change to the sidegrade 30/60/8.0/7° with 0.5
  ammo share.
- Bullet Storm extra projectiles are free; charge 1 extra round each.
- Player speed is uncapped and normalized only after the movement fix; apply
  the 300 px/s cap.
- Reload is instant; add per-weapon duration, indicator, dry-fire, and
  Scavenger reload scaling.
- Explosions fire immediately on death; add the 0.35 s fuse and defer
  `EnterLevelingUp` until all active fuses resolve.
- Damage feedback is silent on absorbed hits and loud on nullified blasts;
  implement outcome-truth cues and blast priority.
- Post-bank kills still award score/scrap; guard on run-active state.
- The vignette is a full-screen wash with no toggle; replace with edges +
  setting.
- Elite tint replaces the type tint; multiply instead.
- Spitter shots are silent; add the charge/fire cue and visible-range gate.

## Open Questions

- Should explosions have falloff instead of a hard 110 px edge?
- Should the damage cooldown be per-source (contact vs projectile vs blast)
  rather than global?
- Is the shared cooldown the right anti-stunlock tool, or should it scale with
  enemy count?
