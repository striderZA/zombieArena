#include "Upgrades.h"

#include <algorithm>

namespace
{
constexpr UpgradeDef kUpgrades[kUpgradeCount] = {
    {"RAPID FIRE", "Fire rate +20%", UpgradeRarity::Common, 5, 100},
    {"EXTENDED MAG", "Clip size +50%", UpgradeRarity::Common, 5, 100},
    {"VITALITY", "+10 max health, heal 10", UpgradeRarity::Common, 5, 100},
    {"SWIFT", "Move speed +10%", UpgradeRarity::Common, 5, 100},
    {"FIELD MEDIC", "Health pickups +50%", UpgradeRarity::Rare, 3, 60},
    {"SCAVENGER", "Ammo pickups +50%", UpgradeRarity::Rare, 3, 60},
    {"HOLLOW POINT", "Bullets deal +1 damage", UpgradeRarity::Rare, 4, 60},
    {"STEADY HANDS", "Bullet spread -20%", UpgradeRarity::Rare, 3, 60},
    {"SHOTGUN CHOKE", "Shotgun +1 pellet", UpgradeRarity::Rare, 3, 55},
    {"ADRENALINE", "Heal 4 HP per kill", UpgradeRarity::Epic, 2, 25},
    {"BULLET STORM", "+1 bullet per shot", UpgradeRarity::Epic, 2, 25},
    {"REPAIR KIT", "Restore 25 health", UpgradeRarity::Common, 999, 0},
};
} // namespace

const UpgradeDef &UpgradeDefinition(int id)
{
    const int index = std::clamp(id, 0, kUpgradeCount - 1);
    return kUpgrades[index];
}
