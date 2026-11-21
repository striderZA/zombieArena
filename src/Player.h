#pragma once

#include "raylib.h"

#include <vector>

class Player
{
public:
    static constexpr float kBodyRadius = 20.0f;

    void SetTexture(Texture2D texture) { texture_ = texture; }

    void ResetStats();
    void Spawn(Rectangle arena, float tileSize, Vector2 offset = {0.0f, 0.0f});

    // moveInput components are -1, 0 or 1. mouseWorld aims the sprite.
    // When obstacles is non-null the player is pushed out of arena cover.
    void Update(float dt, Vector2 moveInput, Vector2 mouseWorld,
                const std::vector<Rectangle> *obstacles = nullptr);

    // Returns true when damage was actually applied (250 ms cooldown).
    bool TakeHit(double now, float damage = 10.0f);

    void UpgradeSpeed();
    void UpgradeHealth();
    void AddMaxHealth(float amount, bool healAddedAmount);
    void MultiplySpeed(float factor);
    void Heal(float amount);
    void Revive(float healthFraction);

    bool Alive() const { return health_ > 0.0f; }

    Rectangle Rect() const;
    Vector2 Center() const { return position_; }
    float Rotation() const { return rotation_; }

    float Health() const { return health_; }
    float MaxHealth() const { return maxHealth_; }
    float Speed() const { return speed_; }

    void Draw(Color tint = WHITE) const;

private:
    static constexpr float kStartSpeed = 200.0f;
    static constexpr float kStartHealth = 100.0f;
    static constexpr double kHitCooldown = 0.25; // seconds
    static constexpr float kSpeedUpgradeFactor = 1.10f;
    static constexpr float kHealthUpgrade = 10.0f;

    void ClampToArena();

    Texture2D texture_{};
    Vector2 position_{};
    Rectangle arena_{};
    float tileSize_ = 50.0f;
    float speed_ = kStartSpeed;
    float health_ = kStartHealth;
    float maxHealth_ = kStartHealth;
    double lastHitTime_ = -1000.0;
    float rotation_ = 0.0f;
};
