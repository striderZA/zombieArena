#pragma once

#include "raylib.h"

class Rng;

class Bullet
{
public:
    static constexpr float kSpeed = 1000.0f;
    static constexpr float kRange = 1000.0f;
    static constexpr float kWidth = 4.0f;
    static constexpr float kHeight = 2.0f;

    // spreadDegrees is the full cone width; the shot is randomised inside it.
    void Shoot(Vector2 from, Vector2 target, float spreadDegrees, Rng &rng, int damage);
    void Update(float dt);
    void Stop() { inFlight_ = false; }

    bool InFlight() const { return inFlight_; }
    Vector2 Position() const { return position_; }
    Vector2 PreviousPosition() const { return previous_; }
    Vector2 Direction() const { return direction_; }
    int Damage() const { return damage_; }
    Rectangle Rect() const;
    float Rotation() const { return rotation_; }

    void Draw() const;

private:
    Vector2 position_{};
    Vector2 previous_{};
    Vector2 velocity_{};
    Vector2 direction_{1.0f, 0.0f};
    float rangeLeft_ = 0.0f;
    float rotation_ = 0.0f;
    int damage_ = 1;
    bool inFlight_ = false;
};
