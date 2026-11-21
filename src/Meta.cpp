#include "Meta.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace
{
constexpr MetaUpgradeDef kMetaUpgrades[kMetaCount] = {
    {"REINFORCED VEST", "+5 starting max health per level", 5, 40, 40},
    {"WEAPON TUNING", "+1 bullet damage per level", 2, 120, 120},
    {"RUNNING SHOES", "+3% move speed per level", 5, 40, 40},
    {"DEEP POCKETS", "+10% ammo from pickups per level", 5, 50, 50},
    {"SCRAP MAGNET", "+20% scrap earned per level", 5, 50, 50},
};
} // namespace

const MetaUpgradeDef &MetaUpgrade(int id)
{
    return kMetaUpgrades[std::clamp(id, 0, kMetaCount - 1)];
}

int MetaUpgradeCost(int id, int currentLevel)
{
    const MetaUpgradeDef &def = MetaUpgrade(id);
    if (currentLevel >= def.maxLevel)
        return -1;
    return def.baseCost + def.costStep * currentLevel;
}

bool LoadProfile(const std::string &path, Profile &profile)
{
    std::ifstream input(path);
    if (!input.is_open())
        return false;

    std::string key;
    int value = 0;
    while (input >> key >> value)
    {
        if (key == "scrap")
            profile.scrap = std::max(0, value);
        else if (key == "runs")
            profile.runs = std::max(0, value);
        else if (key == "kills")
            profile.kills = std::max(0, value);
        else if (key == "best_wave")
            profile.bestWave = std::max(0, value);
        else if (key == "total_scrap")
            profile.totalScrap = std::max(0, value);
        else if (key == "smg")
            profile.smgUnlocked = value != 0;
        else
        {
            for (int id = 0; id < kMetaCount; ++id)
            {
                if (key == std::string("level_") + std::to_string(id))
                {
                    profile.levels[id] = std::clamp(value, 0, MetaUpgrade(id).maxLevel);
                    break;
                }
            }
        }
    }
    return true;
}

bool SaveProfile(const std::string &path, const Profile &profile)
{
    std::ofstream output(path);
    if (!output.is_open())
        return false;

    output << "scrap " << profile.scrap << "\n";
    output << "runs " << profile.runs << "\n";
    output << "kills " << profile.kills << "\n";
    output << "best_wave " << profile.bestWave << "\n";
    output << "total_scrap " << profile.totalScrap << "\n";
    output << "smg " << (profile.smgUnlocked ? 1 : 0) << "\n";
    for (int id = 0; id < kMetaCount; ++id)
        output << "level_" << id << " " << profile.levels[id] << "\n";
    return true;
}
