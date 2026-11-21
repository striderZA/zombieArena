#pragma once

enum class UpgradeRarity
{
    Common,
    Rare,
    Epic
};

struct UpgradeDef
{
    const char *name;
    const char *description;
    UpgradeRarity rarity;
    int maxStacks;
    int weight; // 0 means never offered by the normal draft pool
};

enum UpgradeId : int
{
    kRapidFire = 0,
    kExtendedMag,
    kVitality,
    kSwift,
    kFieldMedic,
    kScavenger,
    kHollowPoint,
    kSteadyHands,
    kShotgunChoke,
    kAdrenaline,
    kBulletStorm,
    kRepairKit,
    kUpgradeCount
};

constexpr int kDraftSize = 3;

const UpgradeDef &UpgradeDefinition(int id);
