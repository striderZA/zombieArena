#pragma once

#include "raylib.h"

#include <cstdint>

class Arena
{
public:
    void SetWave(int wave, std::uint64_t seed);

    Rectangle Bounds() const { return {0.0f, 0.0f, size_, size_}; }
    float Size() const { return size_; }
    int TileSize() const { return kTileSize; }

    void Draw(Texture2D sheet) const;

private:
    static constexpr int kTileSize = 50;
    static constexpr int kTileTypes = 3;
    static constexpr int kBorderRow = 3;

    float size_ = 650.0f;
    std::uint64_t seed_ = 0;
};
