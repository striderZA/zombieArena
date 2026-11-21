#pragma once

#include "raylib.h"

enum class ZombieType
{
    Bloater = 0,
    Chaser = 1,
    Crawler = 2,
    Spitter = 3,
    Sprinter = 4,
    Exploder = 5,
    Boss = 6
};

class Zombie
{
public:
    void Spawn(ZombieType type, Vector2 position, Texture2D texture, Texture2D bloodTexture,
               float speed, int health);

    // Returns true when this hit killed the zombie.
    bool Hit(int damage);
    void Knockback(Vector2 direction, float distance);
    void SetFlash(float seconds) { flash_ = seconds; }

    bool Alive() const { return alive_; }
    bool Ranged() const { return type_ == ZombieType::Spitter; }
    // Spitters return true when their next ranged attack is ready.
    bool TickAttack(float dt);

    void Update(float dt, Vector2 playerPosition);
    void AddSpeed(float amount) { speed_ += amount; }
    void Draw() const;

    Rectangle Rect() const;
    Vector2 Position() const { return position_; }
    float Speed() const { return speed_; }
    float Scale() const { return scale_; }
    int Health() const { return health_; }
    int MaxHealth() const { return maxHealth_; }
    ZombieType Type() const { return type_; }
    float ContactDamage() const { return ContactDamageValue(type_); }

    static constexpr float BaseSpeed(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Bloater: return 40.0f;
        case ZombieType::Chaser: return 70.0f;
        case ZombieType::Crawler: return 20.0f;
        case ZombieType::Spitter: return 30.0f;
        case ZombieType::Sprinter: return 120.0f;
        case ZombieType::Exploder: return 55.0f;
        case ZombieType::Boss: return 30.0f;
        }
        return 40.0f;
    }

    static constexpr int BaseHealth(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Bloater: return 5;
        case ZombieType::Chaser: return 1;
        case ZombieType::Crawler: return 3;
        case ZombieType::Spitter: return 2;
        case ZombieType::Sprinter: return 1;
        case ZombieType::Exploder: return 3;
        case ZombieType::Boss: return 30;
        }
        return 1;
    }

    static constexpr float WaveSpeedBonus(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Bloater: return 50.0f;
        case ZombieType::Chaser: return 70.0f;
        case ZombieType::Crawler: return 30.0f;
        case ZombieType::Spitter: return 15.0f;
        case ZombieType::Sprinter: return 30.0f;
        case ZombieType::Exploder: return 15.0f;
        case ZombieType::Boss: return 10.0f;
        }
        return 0.0f;
    }

    static constexpr float ScaleOf(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Sprinter: return 0.85f;
        case ZombieType::Spitter: return 0.95f;
        case ZombieType::Exploder: return 1.10f;
        case ZombieType::Boss: return 2.20f;
        default: return 1.0f;
        }
    }

    static constexpr Color Tint(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Spitter: return Color{150, 255, 160, 255};
        case ZombieType::Sprinter: return Color{170, 205, 255, 255};
        case ZombieType::Exploder: return Color{255, 180, 110, 255};
        case ZombieType::Boss: return Color{255, 130, 130, 255};
        default: return Color{255, 255, 255, 255};
        }
    }

    static constexpr int ScoreValue(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Spitter:
        case ZombieType::Sprinter:
        case ZombieType::Exploder: return 15;
        case ZombieType::Boss: return 100;
        default: return 10;
        }
    }

    static constexpr int ScrapValue(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Spitter:
        case ZombieType::Sprinter:
        case ZombieType::Exploder: return 2;
        case ZombieType::Boss: return 25;
        default: return 1;
        }
    }

    static constexpr float ContactDamageValue(ZombieType type)
    {
        switch (type)
        {
        case ZombieType::Exploder: return 15.0f;
        case ZombieType::Boss: return 20.0f;
        default: return 10.0f;
        }
    }

private:
    ZombieType type_ = ZombieType::Chaser;
    Vector2 position_{};
    Texture2D texture_{};
    Texture2D bloodTexture_{};
    float speed_ = 0.0f;
    float rotation_ = 0.0f;
    float flash_ = 0.0f;
    float spawnFlash_ = 0.0f;
    float attackTimer_ = 0.0f;
    float scale_ = 1.0f;
    int health_ = 0;
    int maxHealth_ = 0;
    bool alive_ = false;
};
