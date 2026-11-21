#pragma once

#include "raylib.h"

#include <string>

struct KeyBinds
{
    int up = KEY_W;
    int down = KEY_S;
    int left = KEY_A;
    int right = KEY_D;
    int reload = KEY_R;
    int pause = KEY_ENTER;
};

struct Settings
{
    float masterVolume = 0.80f;
    float musicVolume = 0.65f;
    float sfxVolume = 0.90f;
    bool fullscreen = true;
    int windowWidth = 1280;
    int windowHeight = 720;
    bool screenShake = true;
    bool coop = false;
    float aimSensitivity = 1.0f;
    KeyBinds keys;
};

bool LoadSettings(const std::string &path, Settings &settings);
bool SaveSettings(const std::string &path, const Settings &settings);

const char *KeyName(int key);
