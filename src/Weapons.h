#pragma once

#include <array>

enum class WeaponType : int
{
    Pistol = 0,
    Shotgun = 1,
    Rifle = 2,
    Smg = 3
};

constexpr int kWeaponCount = 4;

struct WeaponDef
{
    const char *name;
    int clip;
    int spare;
    float fireRate;   // shots per second
    float spread;     // total cone in degrees
    int pellets;      // projectiles per trigger pull
    float ammoShare;  // fraction of an ammo pickup added to this weapon
};

struct WeaponState
{
    int clip = 0;
    int spare = 0;
};

const WeaponDef &WeaponDefinition(WeaponType type);
const char *WeaponName(WeaponType type);

// Owns the three ammo pools plus the weapon-related upgrade multipliers.
class Arsenal
{
public:
    void Reset();

    WeaponType Current() const { return current_; }
    bool IsUnlocked(WeaponType type) const;
    void SetUnlocked(WeaponType type, bool unlocked);
    void Switch(WeaponType type);
    void Cycle(int direction); // +1 / -1 through unlocked weapons only

    WeaponState &State(WeaponType type) { return states_[static_cast<int>(type)]; }
    const WeaponState &State(WeaponType type) const { return states_[static_cast<int>(type)]; }
    WeaponState &CurrentState() { return State(current_); }
    const WeaponState &CurrentState() const { return State(current_); }

    int EffectiveClip(WeaponType type) const;
    float EffectiveFireRate(WeaponType type) const;

    int ProjectilesForShot(WeaponType type) const;
    int DamagePerProjectile() const { return damage_; }

    void MultiplyFireRate(float factor) { fireRateMultiplier_ *= factor; }
    void MultiplySpread(float factor) { spreadMultiplier_ *= factor; }
    void ApplyClipUpgrade(); // +50% clip and grants the added capacity as ammo
    void AddDamage(int amount) { damage_ += amount; }
    void AddExtraPellet() { extraPellets_++; }
    void AddExtraProjectile() { extraProjectiles_++; }
    void AddHealPerKill(int amount) { healPerKill_ += amount; }

    float FireRateMultiplier() const { return fireRateMultiplier_; }
    float SpreadMultiplier() const { return spreadMultiplier_; }
    int ExtraPellets() const { return extraPellets_; }
    int ExtraProjectiles() const { return extraProjectiles_; }
    int HealPerKill() const { return healPerKill_; }

    int AmmoPickupFor(WeaponType type, int pickupValue) const;

private:
    std::array<WeaponState, kWeaponCount> states_{};
    std::array<bool, kWeaponCount> unlocked_{{true, true, true, true}};
    WeaponType current_ = WeaponType::Pistol;
    float fireRateMultiplier_ = 1.0f;
    float spreadMultiplier_ = 1.0f;
    float clipMultiplier_ = 1.0f;
    int damage_ = 1;
    int extraPellets_ = 0;
    int extraProjectiles_ = 0;
    int healPerKill_ = 0;
};
