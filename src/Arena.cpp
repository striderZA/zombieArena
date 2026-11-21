#include "Arena.h"

#include "ObstacleCollision.h"
#include "Rng.h"

#include <algorithm>
#include <cmath>

void Arena::SetWave(int wave, std::uint64_t seed)
{
    // Matches the original: 650 * (1 + wave * 0.1).
    size_ = 650.0f * (1.0f + static_cast<float>(wave) * 0.1f);
    seed_ = seed ^ (static_cast<std::uint64_t>(wave) * 0x9E3779B97F4A7C15ull);
    BuildObstacles(wave);
}

void Arena::BuildObstacles(int wave)
{
    obstacles_.clear();
    if (wave <= 0)
        return;

    Rng rng(seed_ ^ 0xC0FFEE1234567890ull);

    const int count = std::min(2 + wave / 3, 6);
    const float margin = static_cast<float>(kTileSize * 2);
    const float center = size_ * 0.5f;
    const float centerClear = 160.0f;
    // Spacing exceeds the boss body diameter (88 px) so no two cover pieces
    // form a gap that a large zombie cannot squeeze through.
    const float gap = 90.0f;
    const int maxAttempts = 120;

    for (int i = 0; i < count; ++i)
    {
        // Crates, long barriers and the occasional 2x2 block.
        const int kind = rng.RangeInt(0, 10);
        float width = static_cast<float>(kTileSize);
        float height = static_cast<float>(kTileSize);
        if (kind >= 6 && kind < 9)
        {
            if (rng.RangeInt(0, 2) == 0)
                height = static_cast<float>(kTileSize * 2);
            else
                width = static_cast<float>(kTileSize * 2);
        }
        else if (kind >= 9)
        {
            width = static_cast<float>(kTileSize * 2);
            height = static_cast<float>(kTileSize * 2);
        }

        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            const float x = rng.Range(margin, size_ - margin - width);
            const float y = rng.Range(margin, size_ - margin - height);
            const Rectangle candidate{x, y, width, height};

            // Keep the player spawn and a clear firing lane around it open.
            if (CircleOverlapsRect({center, center}, centerClear, candidate))
                continue;

            bool overlaps = false;
            for (const Rectangle &other : obstacles_)
            {
                const Rectangle expanded{other.x - gap, other.y - gap,
                                         other.width + gap * 2.0f, other.height + gap * 2.0f};
                if (CheckCollisionRecs(candidate, expanded))
                {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps)
            {
                obstacles_.push_back(candidate);
                break;
            }
        }
    }
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

void Arena::DrawObstacles() const
{
    const Color body{72, 80, 90, 255};
    const Color edge{148, 160, 174, 255};
    const Color topLight{196, 206, 218, 90};

    for (const Rectangle &rect : obstacles_)
    {
        DrawRectangleRec(Rectangle{rect.x + 5.0f, rect.y + 7.0f, rect.width, rect.height},
                         Fade(BLACK, 0.35f));
        DrawRectangleRec(rect, body);
        DrawRectangleRec(Rectangle{rect.x + 4.0f, rect.y + 4.0f, rect.width - 8.0f, 5.0f}, topLight);
        DrawRectangleLinesEx(rect, 3.0f, edge);

        // Corner rivets read as man-made cover at a glance.
        const float inset = 7.0f;
        const Vector2 rivets[4] = {
            {rect.x + inset, rect.y + inset},
            {rect.x + rect.width - inset, rect.y + inset},
            {rect.x + inset, rect.y + rect.height - inset},
            {rect.x + rect.width - inset, rect.y + rect.height - inset},
        };
        for (const Vector2 &rivet : rivets)
            DrawCircleV(rivet, 2.0f, edge);
    }
}
