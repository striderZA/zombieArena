#pragma once

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <vector>

// Arena cover is a set of static axis-aligned rectangles. These helpers keep
// the collision rules identical for the player, zombies, bullets and pickups.

// True when a circle centred on `center` touches or overlaps `rect`.
inline bool CircleOverlapsRect(Vector2 center, float radius, Rectangle rect)
{
    const float closestX = std::clamp(center.x, rect.x, rect.x + rect.width);
    const float closestY = std::clamp(center.y, rect.y, rect.y + rect.height);
    const float dx = center.x - closestX;
    const float dy = center.y - closestY;
    const float distanceSq = dx * dx + dy * dy;
    // Contact alone is not overlap: resolution leaves circles exactly on the
    // face, and floating point error must not keep them "blocked".
    return distanceSq < radius * radius * (1.0f - 1e-5f);
}

// Pushes `center` out of `rect` along the shortest path. Returns true when the
// circle was overlapping and the centre moved.
inline bool ResolveCircleVsRect(Vector2 &center, float radius, Rectangle rect)
{
    const float closestX = std::clamp(center.x, rect.x, rect.x + rect.width);
    const float closestY = std::clamp(center.y, rect.y, rect.y + rect.height);
    const float dx = center.x - closestX;
    const float dy = center.y - closestY;
    const float distanceSq = dx * dx + dy * dy;

    if (distanceSq > radius * radius)
        return false;

    if (distanceSq > 1e-8f)
    {
        const float distance = std::sqrt(distanceSq);
        const float push = radius - distance;
        center.x += dx / distance * push;
        center.y += dy / distance * push;
        return true;
    }

    // The centre is inside the rectangle. Leave through the nearest edge.
    const float left = center.x - rect.x;
    const float right = rect.x + rect.width - center.x;
    const float top = center.y - rect.y;
    const float bottom = rect.y + rect.height - center.y;

    float shortest = left;
    int edge = 0;
    if (right < shortest)
    {
        shortest = right;
        edge = 1;
    }
    if (top < shortest)
    {
        shortest = top;
        edge = 2;
    }
    if (bottom < shortest)
        edge = 3;

    switch (edge)
    {
    case 0: center.x = rect.x - radius; break;
    case 1: center.x = rect.x + rect.width + radius; break;
    case 2: center.y = rect.y - radius; break;
    default: center.y = rect.y + rect.height + radius; break;
    }
    return true;
}

// Resolves against every rectangle. Two passes settle corners where the
// circle touches two obstacles at once.
inline void ResolveCircleAgainstObstacles(Vector2 &center, float radius,
                                          const std::vector<Rectangle> &obstacles)
{
    for (int pass = 0; pass < 2; ++pass)
    {
        bool moved = false;
        for (const Rectangle &rect : obstacles)
            moved |= ResolveCircleVsRect(center, radius, rect);
        if (!moved)
            return;
    }
}

inline bool CircleBlockedByObstacles(Vector2 center, float radius,
                                     const std::vector<Rectangle> &obstacles)
{
    for (const Rectangle &rect : obstacles)
    {
        if (CircleOverlapsRect(center, radius, rect))
            return true;
    }
    return false;
}
