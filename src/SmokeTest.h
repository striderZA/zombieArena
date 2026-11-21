#pragma once

#include "Game.h"

#include <string>

// Runs scripted integration passes over the real game loop and writes
// screenshots plus a pass/fail summary. Returns 0 when every check passes.
int RunSmokeTest(Game &game, const std::string &screenshotDir, const GameOptions &baseOptions);
