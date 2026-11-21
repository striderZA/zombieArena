#pragma once

#include <array>
#include <string>

enum MetaId : int
{
    kMetaVest = 0, // +5 max health per level
    kMetaTuning,   // +1 bullet damage per level
    kMetaShoes,    // +3% move speed per level
    kMetaPockets,  // +10% ammo from pickups per level
    kMetaMagnet,   // +20% scrap per level
    kMetaCount
};

constexpr int kSmgUnlockCost = 250;

struct MetaUpgradeDef
{
    const char *name;
    const char *description;
    int maxLevel;
    int baseCost;
    int costStep;
};

const MetaUpgradeDef &MetaUpgrade(int id);
int MetaUpgradeCost(int id, int currentLevel); // -1 when maxed

struct Profile
{
    int scrap = 0;
    int runs = 0;
    int kills = 0;
    int bestWave = 0;
    int totalScrap = 0;
    bool smgUnlocked = false;
    std::array<int, kMetaCount> levels{};
};

bool LoadProfile(const std::string &path, Profile &profile);
bool SaveProfile(const std::string &path, const Profile &profile);
