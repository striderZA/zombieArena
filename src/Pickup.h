#pragma once

#include "Rng.h"
#include "raylib.h"

#include <vector>

class Pickup
{
public:
    enum class Kind
    {
        Health = 1,
        Ammo = 2
    };

    explicit Pickup(Kind kind);

    void SetTexture(Texture2D texture) { texture_ = texture; }
    // Must be called before SetArena so new drops avoid arena cover.
    void SetObstacles(const std::vector<Rectangle> *obstacles) { obstacles_ = obstacles; }
    void Reset();
    void SetArena(Rectangle arena, Rng &rng);
    void Spawn(Rng &rng);
    void ForceSpawn(Vector2 position);
    void Update(float dt, Rng &rng);

    bool IsSpawned() const { return spawned_; }
    int GotIt();
    void Upgrade();

    Rectangle Rect() const;
    Vector2 Position() const { return position_; }
    int Value() const { return value_; }
    void Draw() const;

private:
    static constexpr int kHealthStartValue = 50;
    static constexpr int kAmmoStartValue = 16;
    static constexpr float kStartWaitTime = 6.0f;
    static constexpr float kStartSecondsToLive = 6.0f;

    Kind kind_;
    Texture2D texture_{};
    Rectangle arena_{};
    const std::vector<Rectangle> *obstacles_ = nullptr;
    Vector2 position_{};
    int value_ = 0;
    bool spawned_ = false;
    float secondsSinceSpawn_ = 0.0f;
    float secondsSinceDespawn_ = 0.0f;
    float secondsToLive_ = kStartSecondsToLive;
    float secondsToWait_ = kStartWaitTime;
};
