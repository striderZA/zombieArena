#include "Player.h"

#include <cmath>

void Player::ResetStats()
{
    speed_ = kStartSpeed;
    health_ = kStartHealth;
    maxHealth_ = kStartHealth;
    lastHitTime_ = -1000.0;
    rotation_ = 0.0f;
}

void Player::Spawn(Rectangle arena, float tileSize, Vector2 offset)
{
    arena_ = arena;
    tileSize_ = tileSize;
    position_ = {arena.x + arena.width * 0.5f + offset.x,
                 arena.y + arena.height * 0.5f + offset.y};
}

void Player::Update(float dt, Vector2 moveInput, Vector2 mouseWorld)
{
    // Per-axis movement keeps the original diagonal speed (no normalization).
    position_.x += moveInput.x * speed_ * dt;
    position_.y += moveInput.y * speed_ * dt;

    const float minX = arena_.x + tileSize_;
    const float minY = arena_.y + tileSize_;
    const float maxX = arena_.x + arena_.width - tileSize_;
    const float maxY = arena_.y + arena_.height - tileSize_;

    if (position_.x < minX)
        position_.x = minX;
    if (position_.x > maxX)
        position_.x = maxX;
    if (position_.y < minY)
        position_.y = minY;
    if (position_.y > maxY)
        position_.y = maxY;

    // Aim from the player, not the screen center (the SFML build had this
    // relative to the window, which only matched while centered on screen).
    rotation_ = std::atan2(mouseWorld.y - position_.y, mouseWorld.x - position_.x) * RAD2DEG;
}

bool Player::TakeHit(double now, float damage)
{
    if (now - lastHitTime_ < kHitCooldown)
        return false;

    lastHitTime_ = now;
    health_ -= damage;
    return true;
}

void Player::UpgradeSpeed()
{
    MultiplySpeed(kSpeedUpgradeFactor);
}

void Player::UpgradeHealth()
{
    AddMaxHealth(kHealthUpgrade, true);
}

void Player::AddMaxHealth(float amount, bool healAddedAmount)
{
    maxHealth_ += amount;
    if (healAddedAmount)
        Heal(amount);
}

void Player::MultiplySpeed(float factor)
{
    speed_ *= factor;
}

void Player::Heal(float amount)
{
    if (health_ <= 0.0f)
        return;

    health_ += amount;
    if (health_ > maxHealth_)
        health_ = maxHealth_;
}

void Player::Revive(float healthFraction)
{
    if (health_ > 0.0f)
        return;

    health_ = maxHealth_ * healthFraction;
    if (health_ < 1.0f)
        health_ = 1.0f;
}

Rectangle Player::Rect() const
{
    const float width = texture_.id != 0 ? static_cast<float>(texture_.width) : 50.0f;
    const float height = texture_.id != 0 ? static_cast<float>(texture_.height) : 50.0f;
    return {position_.x - width * 0.5f, position_.y - height * 0.5f, width, height};
}

void Player::Draw(Color tint) const
{
    if (texture_.id == 0)
        return;

    const Rectangle source{0.0f, 0.0f, static_cast<float>(texture_.width), static_cast<float>(texture_.height)};
    const Rectangle dest{position_.x, position_.y, source.width, source.height};
    const Vector2 origin{source.width * 0.5f, source.height * 0.5f};
    DrawTexturePro(texture_, source, dest, origin, rotation_, tint);
}
