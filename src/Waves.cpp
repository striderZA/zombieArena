#include "Waves.h"

#include <algorithm>

int WaveZombieCount(int wave, bool coop)
{
    int count = 5 + (std::max(1, wave) - 1) * 2;
    if (coop)
        count += 2 + std::max(1, wave);

    if (wave > 0 && wave % 5 == 0)
        count = std::max(3, count - 3) + 1; // three regulars trade in for a boss
    return count;
}

std::vector<ZombieType> ComposeWave(int wave, bool coop, Rng &rng)
{
    const int total = WaveZombieCount(wave, coop);
    const bool bossWave = wave > 0 && wave % 5 == 0;
    const int regular = bossWave ? total - 1 : total;

    int weights[7] = {35, 45, 30, 0, 0, 0, 0}; // bloater, chaser, crawler, spitter, sprinter, exploder, boss
    if (wave >= 2)
        weights[static_cast<int>(ZombieType::Spitter)] = 15;
    if (wave >= 3)
        weights[static_cast<int>(ZombieType::Sprinter)] = 12;
    if (wave >= 4)
        weights[static_cast<int>(ZombieType::Exploder)] = 10;

    std::vector<ZombieType> types;
    types.reserve(static_cast<std::size_t>(total));

    if (bossWave)
        types.push_back(ZombieType::Boss);

    for (int i = 0; i < regular; ++i)
    {
        int totalWeight = 0;
        for (int w = 0; w < 6; ++w)
            totalWeight += weights[w];

        int roll = rng.RangeInt(0, totalWeight);
        int picked = 0;
        for (int w = 0; w < 6; ++w)
        {
            if (roll < weights[w])
            {
                picked = w;
                break;
            }
            roll -= weights[w];
        }
        types.push_back(static_cast<ZombieType>(picked));
    }

    return types;
}
