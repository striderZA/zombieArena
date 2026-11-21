#include "Weapons.h"

#include <algorithm>
#include <cmath>

namespace
{
// Spawn-ammo values mirror the original pistol economy; shotgun and rifle
// trade capacity for damage and rate of fire.
constexpr WeaponDef kWeapons[kWeaponCount] = {
    // name       clip spare fireRate spread pellets ammoShare
    {"PISTOL",     6,   48,  1.0f,    2.0f,  1,      1.00f},
    {"SHOTGUN",    2,   12,  0.8f,   20.0f,  6,      0.35f},
    {"RIFLE",     20,   90,  4.0f,    5.0f,  1,      1.00f},
    {"SMG",       30,  120,  8.0f,    4.0f,  1,      1.00f},
};
} // namespace

const WeaponDef &WeaponDefinition(WeaponType type)
{
    const int index = std::clamp(static_cast<int>(type), 0, kWeaponCount - 1);
    return kWeapons[index];
}

const char *WeaponName(WeaponType type)
{
    return WeaponDefinition(type).name;
}

void Arsenal::Reset()
{
    for (int i = 0; i < kWeaponCount; ++i)
    {
        states_[i].clip = kWeapons[i].clip;
        states_[i].spare = kWeapons[i].spare;
    }
    current_ = WeaponType::Pistol;
    fireRateMultiplier_ = 1.0f;
    spreadMultiplier_ = 1.0f;
    clipMultiplier_ = 1.0f;
    damage_ = 1;
    extraPellets_ = 0;
    extraProjectiles_ = 0;
    healPerKill_ = 0;
}

bool Arsenal::IsUnlocked(WeaponType type) const
{
    const int index = static_cast<int>(type);
    return index >= 0 && index < kWeaponCount && unlocked_[index];
}

void Arsenal::SetUnlocked(WeaponType type, bool unlocked)
{
    const int index = static_cast<int>(type);
    if (index < 0 || index >= kWeaponCount)
        return;

    unlocked_[index] = unlocked;
    if (!unlocked && current_ == type)
        current_ = WeaponType::Pistol;
}

void Arsenal::Switch(WeaponType type)
{
    const int index = static_cast<int>(type);
    if (index >= 0 && index < kWeaponCount && unlocked_[index])
        current_ = type;
}

void Arsenal::Cycle(int direction)
{
    if (direction == 0)
        return;

    int index = static_cast<int>(current_);
    for (int attempt = 0; attempt < kWeaponCount; ++attempt)
    {
        index = (index + (direction > 0 ? 1 : kWeaponCount - 1)) % kWeaponCount;
        if (unlocked_[index])
        {
            current_ = static_cast<WeaponType>(index);
            return;
        }
    }
}

int Arsenal::EffectiveClip(WeaponType type) const
{
    const int base = WeaponDefinition(type).clip;
    const int value = static_cast<int>(std::lround(static_cast<float>(base) * clipMultiplier_));
    return std::max(1, value);
}

float Arsenal::EffectiveFireRate(WeaponType type) const
{
    return WeaponDefinition(type).fireRate * fireRateMultiplier_;
}

int Arsenal::ProjectilesForShot(WeaponType type) const
{
    const WeaponDef &def = WeaponDefinition(type);
    int count = def.pellets + extraProjectiles_;
    if (type == WeaponType::Shotgun)
        count += extraPellets_;
    return std::max(1, count);
}

void Arsenal::ApplyClipUpgrade()
{
    int oldClips[kWeaponCount];
    for (int i = 0; i < kWeaponCount; ++i)
        oldClips[i] = EffectiveClip(static_cast<WeaponType>(i));

    clipMultiplier_ += 0.5f;

    for (int i = 0; i < kWeaponCount; ++i)
    {
        const WeaponType type = static_cast<WeaponType>(i);
        const int newClip = EffectiveClip(type);
        WeaponState &state = states_[i];
        state.clip = std::min(newClip, state.clip + (newClip - oldClips[i]));
    }
}

int Arsenal::AmmoPickupFor(WeaponType type, int pickupValue) const
{
    const float share = WeaponDefinition(type).ammoShare;
    return std::max(0, static_cast<int>(std::lround(static_cast<float>(pickupValue) * share)));
}
