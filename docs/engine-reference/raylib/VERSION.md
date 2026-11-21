# Raylib Engine — Version Reference

| Field | Value |
|-------|-------|
| **Engine Version** | Raylib 6.2 |
| **Release Date** | 2026 |
| **Project Pinned** | 2026-11-21 |
| **Last Docs Verified** | 2026-11-21 |
| **LLM Knowledge Cutoff** | May 2025 |

## Knowledge Gap Warning

Raylib 6.2 is **after** the model training cutoff. Treat this reference as
orientation only and verify every non-trivial API call against the vendored
header at `third_party/raylib/include/raylib.h` and the upstream raylib
examples/changelog. Do not assume 6.2 signatures from 5.x memory.

## Risk Level: MEDIUM

The core raylib API is stable and generally backwards-compatible, but 6.x
changes and build flags are post-cutoff. Compile early and prefer the local
header over recalled API shapes.

## Version History

| Version | Release | Risk Level | Key Theme |
|---------|---------|------------|-----------|
| 4.x | 2022-2023 | LOW | Stable API, widely documented |
| 5.0 | 2024 | LOW | Minor improvements, backwards compatible |
| 5.5 | 2025 | LOW | Last release within training data |
| 6.2 | 2026 | MEDIUM | Project-pinned; verify against vendored headers |

## Raylib Characteristics

- **C library** with PascalCase function names (`InitWindow`, `LoadTexture`, etc.)
- **Header-only modules**: raymath, raygui (optional)
- **Platform support**: Windows, macOS, Linux, Web (Emscripten), Android, Raspberry Pi
- **No editor** — code-only development
- **Simple API** designed for learning and prototyping
- **zlib/libpng license** — free with no restrictions
- **No built-in physics, scene graph, or UI system** — bring your own

## Key Modules

| Module | Purpose | Header |
|--------|---------|--------|
| core | Window, input, timing, file I/O | `raylib.h` |
| rlgl | Raw OpenGL abstraction | `raylib.h` (advanced) |
| raudio | Audio device, sound, music | `raylib.h` |
| raymath | Vector/Matrix math | `raymath.h` |
| raygui | Immediate-mode UI | `raygui.h` |

## Project-Specific Notes

- This project uses raylib's 2D path: `Camera2D`, textures, shapes, and raudio.
- `third_party/raylib/lib/libraylib.a` is the Windows MinGW-w64 build used by
  CMake; do not replace it without rebuilding and running the full test suite.
- There are no custom shaders or raygui screens yet.

## Verified Sources

- Local authoritative header: `third_party/raylib/include/raylib.h`
- Official website: https://www.raylib.com/
- GitHub repository: https://github.com/raysan5/raylib
- Wiki: https://github.com/raysan5/raylib/wiki
- Cheatsheet: https://www.raylib.com/cheatsheet/cheatsheet.html
- Examples: https://github.com/raysan5/raylib/tree/master/examples
