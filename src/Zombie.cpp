#include "Zombie.h"

#include "ObstacleCollision.h"

#include <algorithm>
#include <cmath>

void Zombie::Spawn(ZombieType type, Vector2 position, Texture2D texture, Texture2D bloodTexture,
                   float speed, int health, bool elite)
{
    type_ = type;
    position_ = position;
    texture_ = texture;
    bloodTexture_ = bloodTexture;
    elite_ = elite && type != ZombieType::Boss;
    speed_ = speed * (elite_ ? 1.15f : 1.0f);
    health_ = elite_ ? health * 2 : health;
    maxHealth_ = health_;
    scale_ = ScaleOf(type) * (elite_ ? 1.25f : 1.0f);
    attackTimer_ = 1.2f;
    alive_ = true;
    rotation_ = 0.0f;
    flash_ = 0.0f;
    spawnFlash_ = 0.6f;
}

void Zombie::Knockback(Vector2 direction, float distance)
{
    if (!alive_)
        return;

    const float resistance = 1.0f / (scale_ * scale_);
    position_.x += direction.x * distance * resistance;
    position_.y += direction.y * distance * resistance;
}

bool Zombie::Hit(int damage)
{
    // The original dies on the hit that takes health below zero, which is why
    // a health-5 bloater needs six points of damage.
    health_ -= damage;
    flash_ = 0.08f;

    if (health_ < 0)
    {
        alive_ = false;
        texture_ = bloodTexture_;
        rotation_ = 0.0f;
        flash_ = 0.0f;
        if (scale_ > 1.6f)
            scale_ = 1.6f;
        return true;
    }
    return false;
}

bool Zombie::TickAttack(float dt)
{
    if (!alive_ || !Ranged())
        return false;

    attackTimer_ -= dt;
    if (attackTimer_ <= 0.0f)
    {
        attackTimer_ = 2.2f;
        return true;
    }
    return false;
}

void Zombie::Update(float dt, Vector2 playerPosition,
                    const std::vector<Rectangle> *obstacles)
{
    if (flash_ > 0.0f)
        flash_ = flash_ > dt ? flash_ - dt : 0.0f;
    if (spawnFlash_ > 0.0f)
        spawnFlash_ = spawnFlash_ > dt ? spawnFlash_ - dt : 0.0f;

    if (!alive_)
        return;

    const float dx = playerPosition.x - position_.x;
    const float dy = playerPosition.y - position_.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    auto move = [&](float moveX, float moveY) {
        position_.x += moveX;
        position_.y += moveY;
        if (obstacles != nullptr)
            ResolveCircleAgainstObstacles(position_, Radius(), *obstacles);
    };

    if (Ranged())
    {
        // Spitters keep their distance instead of hugging the player.
        if (distance > 250.0f && distance > 0.001f)
            move(dx / distance * speed_ * dt, dy / distance * speed_ * dt);
        else if (distance < 150.0f && distance > 0.001f)
            move(-dx / distance * speed_ * dt, -dy / distance * speed_ * dt);
    }
    else
    {
        // Per-axis chase, as in the original: diagonal movement is faster.
        // Each axis resolves separately so cover makes zombies slide around
        // its edges instead of stalling against the first wall.
        const float step = speed_ * dt;
        if (playerPosition.x > position_.x)
            move(step, 0.0f);
        else if (playerPosition.x < position_.x)
            move(-step, 0.0f);

        if (playerPosition.y > position_.y)
            move(0.0f, step);
        else if (playerPosition.y < position_.y)
            move(0.0f, -step);
    }

    rotation_ = std::atan2(playerPosition.y - position_.y, playerPosition.x - position_.x) * RAD2DEG;
}

void Zombie::ResolveObstacles(const std::vector<Rectangle> &obstacles)
{
    if (!alive_)
        return;
    ResolveCircleAgainstObstacles(position_, Radius(), obstacles);
}

float Zombie::Radius() const
{
    const float width = texture_.id != 0 ? static_cast<float>(texture_.width) : 50.0f;
    return width * scale_ * 0.4f;
}

Rectangle Zombie::Rect() const
{
    const float width = (texture_.id != 0 ? static_cast<float>(texture_.width) : 50.0f) * scale_;
    const float height = (texture_.id != 0 ? static_cast<float>(texture_.height) : 50.0f) * scale_;
    return {position_.x - width * 0.5f, position_.y - height * 0.5f, width, height};
}

void Zombie::Draw() const
{
    if (texture_.id == 0)
        return;

    if (alive_ && spawnFlash_ > 0.0f)
    {
        const float pulse = 1.0f - spawnFlash_ / 0.6f;
        DrawCircleLinesV(position_, (28.0f + pulse * 16.0f) * scale_, Fade(RED, spawnFlash_ / 0.6f));
    }

    if (alive_ && type_ == ZombieType::Boss)
        DrawCircleLinesV(position_, 42.0f * scale_, Fade(RED, 0.35f));

    if (alive_ && elite_)
    {
        const float pulse = 0.65f + 0.35f * std::sin(static_cast<float>(GetTime()) * 6.0f);
        DrawCircleLinesV(position_, Radius() + 6.0f, Fade(GOLD, 0.75f * pulse));
        DrawCircleLinesV(position_, Radius() + 12.0f, Fade(GOLD, 0.30f * pulse));
    }

    const Rectangle source{0.0f, 0.0f, static_cast<float>(texture_.width), static_cast<float>(texture_.height)};
    const float size = scale_;
    const Rectangle dest{position_.x, position_.y, source.width * size, source.height * size};
    const Vector2 origin{source.width * size * 0.5f, source.height * size * 0.5f};
    Color tint = alive_ ? Tint(type_) : WHITE;
    if (alive_ && elite_)
        tint = Color{255, 224, 140, 255};
    if (alive_ && flash_ > 0.0f)
        tint = Color{255, 120, 120, 255};
    DrawTexturePro(texture_, source, dest, origin, rotation_, tint);
}
