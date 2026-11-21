#pragma once

#include <cstdint>

// Small deterministic RNG (splitmix64). Replaces the srand(time(...)) calls
// from the SFML original so a run is reproducible from its seed.
class Rng
{
public:
    explicit Rng(std::uint64_t seed = 0x9E3779B97F4A7C15ull) : state_(seed) {}

    std::uint64_t Next()
    {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    // [lo, hi)
    int RangeInt(int lo, int hi)
    {
        if (hi <= lo)
            return lo;
        return lo + static_cast<int>(Next() % static_cast<std::uint64_t>(hi - lo));
    }

    // [lo, hi)
    float Range(float lo, float hi)
    {
        const float unit = static_cast<float>(Next() >> 40) / 16777216.0f;
        return lo + unit * (hi - lo);
    }

private:
    std::uint64_t state_;
};

// Stable hash for tile variation (no RNG state consumed while drawing).
inline std::uint32_t Hash2D(int x, int y, std::uint64_t seed)
{
    std::uint64_t h = seed;
    h ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * 0x9E3779B97F4A7C15ull;
    h ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) * 0xC2B2AE3D27D4EB4Full;
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDull;
    h ^= h >> 33;
    return static_cast<std::uint32_t>(h);
}
