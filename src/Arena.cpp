#include "Arena.h"

#include "Rng.h"

#include <cmath>

void Arena::SetWave(int wave, std::uint64_t seed)
{
    // Matches the original: 650 * (1 + wave * 0.1).
    size_ = 650.0f * (1.0f + static_cast<float>(wave) * 0.1f);
    seed_ = seed ^ (static_cast<std::uint64_t>(wave) * 0x9E3779B97F4A7C15ull);
}

void Arena::Draw(Texture2D sheet) const
{
    if (sheet.id == 0)
        return;

    const int worldWidth = static_cast<int>(std::ceil(size_ / static_cast<float>(kTileSize)));
    const int worldHeight = worldWidth;

    const Rectangle borderSource{
        0.0f,
        static_cast<float>(kBorderRow * kTileSize),
        static_cast<float>(kTileSize),
        static_cast<float>(kTileSize)};

    for (int w = 0; w < worldWidth; ++w)
    {
        for (int h = 0; h < worldHeight; ++h)
        {
            const bool border = (h == 0) || (h == worldHeight - 1) || (w == 0) || (w == worldWidth - 1);

            Rectangle source = borderSource;
            if (!border)
            {
                const int row = static_cast<int>(Hash2D(w, h, seed_) % kTileTypes);
                source.y = static_cast<float>(row * kTileSize);
            }

            const Vector2 position{
                static_cast<float>(w * kTileSize),
                static_cast<float>(h * kTileSize)};
            DrawTextureRec(sheet, source, position, WHITE);
        }
    }
}
