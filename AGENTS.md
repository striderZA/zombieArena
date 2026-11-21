# Zombie Arena

Indie game development managed through the OpenCode Game Studios (OCGS) agent
roster. This project is a C++17/raylib port of the SFML **Zombie Arena** sample.

## Technology Stack

- **Engine**: Raylib 6.2 (vendored under `third_party/raylib`)
- **Language**: C++17
- **Build System**: CMake 3.20+ with Ninja and MinGW-w64 (tested GCC 15.2.0)
- **Asset Pipeline**: PNG/WAV/TTF files in `assets/`, copied next to the
  executable by CMake; save data stays in `assets/gamedata/`
- **Agent Harness**: oh-my-pi (omp 18.16+)
- **Tests**: CTest (`logic_tests` + scripted `smoke` with screenshots)

## Project Structure

```text
/
├── AGENTS.md                    # Project configuration
├── CMakeLists.txt               # Windows/MinGW build; raylib is vendored
├── src/                         # C++ game source
│   ├── Game.{h,cpp}             # States, loop, combat, HUD, co-op, settings
│   ├── Player, Zombie, Bullet, Pickup, Arena, Assets, Weapons, Upgrades,
│   │   Waves, Meta, Settings    # Simulation modules
│   └── SweptCollision.h         # Segment-vs-rect swept collision helper
├── assets/                      # Sprites, tiles, WAVs, TTF, high score
├── tests/logic_tests.cpp        # Window-free logic assertions
├── third_party/raylib/          # raylib 6.2 headers + libraylib.a
├── design/                      # GDD space + entity registry
├── docs/                        # Architecture, engine reference, framework docs
├── production/                  # Stage and session state
├── .agents/                     # Canonical OCGS agents/skills/commands/rules
├── .omp/                        # omp config and generated agent definitions
└── build/                       # Generated build tree; ignored
```

## Build and Test

Run from the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/ZombieArenaRaylib.exe
```

Quality gates before declaring work complete:

- Configure and build with no warnings.
- `ctest` passes 2/2 suites (`logic`, `smoke`).
- `node tests/agents/validate.mjs` passes the OCGS content validation.
- UI or behavior changes have a screenshot or scripted assertion that proves
  the change; compare expected to actual before reporting success.

## Installed OCGS Modules

Installed with `--harness omp`:

- `core`
- `engine-raylib`
- `programming`
- `qa`
- `design`
- `architecture`

`technical-artist` and `sound-designer` are included as cross-cutting agents
because the installed `team-combat` and `team-polish` skills reference them.
The full `art` and `audio` modules are not installed. Add a module with:

```sh
node .opencode/modules/install.mjs add <module> --harness omp
```

The canonical content is in `.agents/`; omp agents are generated into
`.omp/agents/`. Other harness configurations are retained so the project can
switch harnesses later.

## Installed Agents

- **Directors**: `creative-director`, `technical-director`, `producer`
- **Leads**: `lead-programmer`, `game-designer`, `qa-lead`
- **Specialists**: `gameplay-programmer`, `engine-programmer`,
  `ai-programmer`, `network-programmer`, `tools-programmer`,
  `systems-designer`, `economy-designer`, `qa-tester`,
  `performance-analyst`, `security-engineer`, `raylib-specialist`,
  `technical-artist`, `sound-designer`

Use omp's `task` tool with the agent name when delegating. Do not invent agents
that are not in `.omp/agents/`.

## Studio Hierarchy

```text
Tier 1 — Directors
  creative-director    technical-director    producer

Tier 2 — Department Leads
  game-designer        lead-programmer       qa-lead

Tier 3 — Specialists
  gameplay-programmer  engine-programmer     ai-programmer
  network-programmer   tools-programmer      systems-designer
  economy-designer     qa-tester             performance-analyst
  security-engineer    raylib-specialist     technical-artist
  sound-designer
```

## Coordination Rules

1. **Vertical Delegation**: Leadership agents delegate to department leads, who
   delegate to specialists. Never skip a tier for complex decisions.
2. **Horizontal Consultation**: Agents at the same tier may consult each other
   but must not make binding decisions outside their domain.
3. **Conflict Resolution**: Escalate to the shared parent. If no shared parent,
   escalate to `creative-director` for design conflicts or
   `technical-director` for technical conflicts.
4. **Change Propagation**: The `producer` coordinates changes affecting
   multiple domains.
5. **No Unilateral Cross-Domain Changes**: Do not modify files outside the
   designated domain without explicit delegation.

## Collaboration Protocol

User-driven collaboration, not autonomous execution:

**Question -> Options -> Decision -> Draft -> Approval**

- Show drafts or summaries before writing multi-file changes.
- Ask before writing to files outside the current task's scope.
- No commits without user instruction.
- Preserve the user's existing uncommitted work and test artifacts.

## Coding Standards

- Public APIs require doc comments.
- New gameplay systems require tests and an ADR in `docs/architecture/`.
- Prefer data-driven tuning for new systems. The existing raylib port still
  contains constants in `src/`; migrate them incrementally, not in unrelated
  changes.
- Keep gameplay deterministic under a fixed seed where possible; the smoke
  suite relies on this.
- C++ style: 4-space indentation, PascalCase types/functions, trailing
  underscore for member data, `kPascalCase` constants. Follow the surrounding
  raylib C API usage.
- Tests live in `tests/` or in the scripted smoke suite; do not place tests in
  `src/`.
- Do not add runtime dependencies beyond raylib and the C++17 standard library
  without an ADR.

## Engine Notes

- The pinned raylib is 6.2. Treat `third_party/raylib/include/raylib.h` as the
  authoritative API and read `docs/engine-reference/raylib/` before using an
  API that may be post-training-cutoff.
- Rendering is 2D only. The game uses `Camera2D`, textures, shapes, and raudio;
  it does not use raygui or 3D models.
- Colyseus-style scene graphs and physics engines are intentionally absent;
  collision is custom AABB/circle logic.

## Context Management

The files are the memory, not the conversation.

- `production/stage.txt` is the authoritative project phase.
- `production/session-state/active.md` is a living checkpoint (gitignored):
  update it after design approvals, architecture decisions, implementation
  milestones, and test results.
- `design/registry/entities.yaml` is the cross-document fact registry.
- Keep `docs/architecture/` ADRs current when systems change.

## Workflow Modes

- **Hybrid workflow** for discovery and small changes: prototype quickly, then
  promote proven work to the production structure.
- **Full OCGS workflow** for features that affect multiple systems, release
  work, or anything needing design -> architecture -> stories -> code -> QA
  gates.

The project is currently in the Production phase; existing code predates OCGS
and is being brought under formal design/architecture documentation
incrementally.

## Getting Started

Start omp from the repository root:

```sh
omp
```

Then use `/` to browse installed commands or `/skill:<name>` for a workflow.
Useful installed workflows include `/start`, `/help`,
`/project-stage-detect`, `/setup-engine`, `/map-systems`, `/design-system`,
`/design-review`, `/architecture-decision`, `/architecture-review`,
`/create-architecture`, `/create-control-manifest`, `/qa-plan`,
`/smoke-check`, `/regression-suite`, `/test-evidence-review`, `/bug-report`,
`/team-qa`, `/team-polish`, and `/team-combat`.

OCGS content added during a session may require a new omp session (or an agent
hub reload) before it appears.

## Quality Gates

- Clean CMake configure/build.
- `ctest` 2/2 suites pass.
- OCGS content validation passes.
- New behavior has a test or documented manual/screenshot verification.
- No credentials, generated build output, or large unapproved binaries in the
  commit.
