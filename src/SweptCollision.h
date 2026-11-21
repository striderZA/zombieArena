#pragma once

#include "raylib.h"

#include <algorithm>
#include <cmath>

// Sweeps the line segment start -> end against an axis-aligned rectangle.
// Returns true when they touch or overlap. entryT receives the normalized
// first contact time in [0, 1]; a segment starting inside the rectangle
// returns 0. This lets a fast bullet test its whole frame movement instead
// of only its final position.
inline bool SegmentIntersectsRect(Vector2 start, Vector2 end, Rectangle rect, float *entryT = nullptr)
{
    float tMin = 0.0f;
    float tMax = 1.0f;
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;

    auto clip = [&](float origin, float delta, float minValue, float maxValue) {
        if (std::fabs(delta) < 1e-6f)
            return origin >= minValue && origin <= maxValue;

        float t1 = (minValue - origin) / delta;
        float t2 = (maxValue - origin) / delta;
        if (t1 > t2)
            std::swap(t1, t2);

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    if (!clip(start.x, dx, rect.x, rect.x + rect.width))
        return false;
    if (!clip(start.y, dy, rect.y, rect.y + rect.height))
        return false;

    if (entryT != nullptr)
        *entryT = tMin;
    return true;
}
