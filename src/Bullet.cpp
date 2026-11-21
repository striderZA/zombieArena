#include "Bullet.h"

#include "Rng.h"

#include <cmath>

void Bullet::Shoot(Vector2 from, Vector2 target, float spreadDegrees, Rng &rng, int damage)
{
    position_ = from;
    previous_ = from;

    float angle = std::atan2(target.y - from.y, target.x - from.x);
    if (std::fabs(target.x - from.x) + std::fabs(target.y - from.y) < 0.0001f)
        angle = 0.0f;

    if (spreadDegrees > 0.0f)
    {
        const float halfCone = spreadDegrees * 0.5f * DEG2RAD;
        angle += rng.Range(-halfCone, halfCone);
    }

    direction_ = {std::cos(angle), std::sin(angle)};
    velocity_ = {direction_.x * kSpeed, direction_.y * kSpeed};
    rotation_ = angle * RAD2DEG;
    rangeLeft_ = kRange;
    damage_ = damage;
    inFlight_ = true;
}

void Bullet::Update(float dt)
{
    if (!inFlight_)
        return;

    previous_ = position_;

    const Vector2 step{velocity_.x * dt, velocity_.y * dt};
    position_.x += step.x;
    position_.y += step.y;

    const float travelled = std::sqrt(step.x * step.x + step.y * step.y);
    rangeLeft_ -= travelled;
    if (rangeLeft_ <= 0.0f)
        inFlight_ = false;
}

Rectangle Bullet::Rect() const
{
    return {position_.x - kWidth * 0.5f, position_.y - kHeight * 0.5f, kWidth, kHeight};
}

void Bullet::Draw() const
{
    if (!inFlight_)
        return;

    const Rectangle rect{position_.x, position_.y, kWidth, kHeight};
    DrawRectanglePro(rect, {kWidth * 0.5f, kHeight * 0.5f}, rotation_, WHITE);
}
