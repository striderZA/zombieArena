#include "Pickup.h"

#include "ObstacleCollision.h"

#include <cmath>

Pickup::Pickup(Kind kind) : kind_(kind)
{
    Reset();
}

void Pickup::Reset()
{
    value_ = (kind_ == Kind::Health) ? kHealthStartValue : kAmmoStartValue;
    secondsToLive_ = kStartSecondsToLive;
    secondsToWait_ = kStartWaitTime;
    secondsSinceSpawn_ = 0.0f;
    secondsSinceDespawn_ = 0.0f;
    spawned_ = false;
}

void Pickup::SetArena(Rectangle arena, Rng &rng)
{
    arena_ = arena;
    Spawn(rng);
}

void Pickup::Spawn(Rng &rng)
{
    const float inset = 50.0f;
    const float minX = arena_.x + inset;
    const float minY = arena_.y + inset;
    const float spanX = arena_.width - inset * 2.0f;
    const float spanY = arena_.height - inset * 2.0f;

    // Try a handful of spots so the drop does not land inside cover.
    for (int attempt = 0; attempt < 32; ++attempt)
    {
        position_.x = minX + rng.Range(0.0f, spanX > 1.0f ? spanX : 1.0f);
        position_.y = minY + rng.Range(0.0f, spanY > 1.0f ? spanY : 1.0f);
        if (obstacles_ == nullptr || !CircleBlockedByObstacles(position_, 26.0f, *obstacles_))
            break;
    }

    secondsSinceSpawn_ = 0.0f;
    spawned_ = true;
}

void Pickup::ForceSpawn(Vector2 position)
{
    position_ = position;
    if (obstacles_ != nullptr)
        ResolveCircleAgainstObstacles(position_, 26.0f, *obstacles_);
    secondsSinceSpawn_ = 0.0f;
    spawned_ = true;
}

void Pickup::Update(float dt, Rng &rng)
{
    if (spawned_)
    {
        secondsSinceSpawn_ += dt;
        if (secondsSinceSpawn_ > secondsToLive_)
        {
            spawned_ = false;
            secondsSinceDespawn_ = 0.0f;
        }
    }
    else
    {
        secondsSinceDespawn_ += dt;
        if (secondsSinceDespawn_ > secondsToWait_)
            Spawn(rng);
    }
}

int Pickup::GotIt()
{
    spawned_ = false;
    secondsSinceDespawn_ = 0.0f;
    return value_;
}

void Pickup::Upgrade()
{
    if (kind_ == Kind::Health)
        value_ += kHealthStartValue / 2;
    else
        value_ += kAmmoStartValue / 2;

    secondsToLive_ += kStartSecondsToLive / 10.0f;
    secondsToWait_ -= kStartWaitTime / 10.0f;
    if (secondsToWait_ < 1.0f)
        secondsToWait_ = 1.0f;
}

Rectangle Pickup::Rect() const
{
    const float width = texture_.id != 0 ? static_cast<float>(texture_.width) : 50.0f;
    const float height = texture_.id != 0 ? static_cast<float>(texture_.height) : 50.0f;
    return {position_.x - width * 0.5f, position_.y - height * 0.5f, width, height};
}

void Pickup::Draw() const
{
    if (!spawned_ || texture_.id == 0)
        return;

    const Rectangle source{0.0f, 0.0f, static_cast<float>(texture_.width), static_cast<float>(texture_.height)};
    const Rectangle dest{position_.x, position_.y, source.width, source.height};
    const Vector2 origin{source.width * 0.5f, source.height * 0.5f};
    DrawTexturePro(texture_, source, dest, origin, 0.0f, WHITE);
}
