#pragma once

#include "raylib.h"

#include <cstdint>
#include <vector>

class Arena
{
public:
    void SetWave(int wave, std::uint64_t seed);

    Rectangle Bounds() const { return {0.0f, 0.0f, size_, size_}; }
    float Size() const { return size_; }
    int TileSize() const { return kTileSize; }

    // Static cover generated for the current wave. Deterministic for a given
    // wave and seed, so runs and the smoke suite stay reproducible.
    const std::vector<Rectangle> &Obstacles() const { return obstacles_; }

    void Draw(Texture2D sheet) const;
    void DrawObstacles() const;

private:
    static constexpr int kTileSize = 50;
    static constexpr int kTileTypes = 3;
    static constexpr int kBorderRow = 3;

    void BuildObstacles(int wave);

    float size_ = 650.0f;
    std::uint64_t seed_ = 0;
    std::vector<Rectangle> obstacles_;
};
