#pragma once

#include "Rng.h"
#include "Zombie.h"

#include <vector>

int WaveZombieCount(int wave, bool coop);
std::vector<ZombieType> ComposeWave(int wave, bool coop, Rng &rng);
