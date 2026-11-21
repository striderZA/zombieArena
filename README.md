# Zombie Arena

> A raylib 2D top-down wave shooter: four weapons, seven enemy types, randomized
> upgrade drafts, persistent scrap, local co-op, gamepad support, and a boss
> every five waves.

A C++17/raylib port of the SFML **Zombie Arena** sample. The original SFML
reference project was removed when the game moved to the repository root; it
remains in git history.

## Build

Prerequisites (Windows):

- CMake 3.20+
- Ninja
- MinGW-w64 GCC with C++17 (tested with GCC 15.2.0)

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ZombieArenaRaylib.exe
```

raylib 6.2 is vendored under `third_party/raylib`; the build copies `assets/`
next to the executable. On other platforms, install raylib and configure with
`-DRAYLIB_ROOT=/path/to/raylib`.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

`logic_tests` covers deterministic simulation rules; `smoke` runs scripted
gameplay and writes screenshots to `build/smoke/`.

## Documentation

See [`docs/raylib-port.md`](docs/raylib-port.md) for controls, progression,
save files, run options, and verification details.
