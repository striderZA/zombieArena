#include "Settings.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

namespace
{
float ClampVolume(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

bool ValidKey(int key)
{
    return key > 0 && key != KEY_ESCAPE;
}
} // namespace

bool LoadSettings(const std::string &path, Settings &settings)
{
    std::ifstream input(path);
    if (!input.is_open())
        return false;

    std::string key;
    float value = 0.0f;
    while (input >> key >> value)
    {
        if (key == "master_volume")
            settings.masterVolume = ClampVolume(value);
        else if (key == "music_volume")
            settings.musicVolume = ClampVolume(value);
        else if (key == "sfx_volume")
            settings.sfxVolume = ClampVolume(value);
        else if (key == "fullscreen")
            settings.fullscreen = value != 0.0f;
        else if (key == "window_width")
            settings.windowWidth = std::clamp(static_cast<int>(value), 800, 3840);
        else if (key == "window_height")
            settings.windowHeight = std::clamp(static_cast<int>(value), 450, 2160);
        else if (key == "screen_shake")
            settings.screenShake = value != 0.0f;
        else if (key == "coop")
            settings.coop = value != 0.0f;
        else if (key == "aim_sensitivity")
            settings.aimSensitivity = std::clamp(value, 0.5f, 2.0f);
        else if (key == "key_up" && ValidKey(static_cast<int>(value)))
            settings.keys.up = static_cast<int>(value);
        else if (key == "key_down" && ValidKey(static_cast<int>(value)))
            settings.keys.down = static_cast<int>(value);
        else if (key == "key_left" && ValidKey(static_cast<int>(value)))
            settings.keys.left = static_cast<int>(value);
        else if (key == "key_right" && ValidKey(static_cast<int>(value)))
            settings.keys.right = static_cast<int>(value);
        else if (key == "key_reload" && ValidKey(static_cast<int>(value)))
            settings.keys.reload = static_cast<int>(value);
        else if (key == "key_pause" && ValidKey(static_cast<int>(value)))
            settings.keys.pause = static_cast<int>(value);
    }
    return true;
}

bool SaveSettings(const std::string &path, const Settings &settings)
{
    std::ofstream output(path);
    if (!output.is_open())
        return false;

    output << "master_volume " << settings.masterVolume << "\n";
    output << "music_volume " << settings.musicVolume << "\n";
    output << "sfx_volume " << settings.sfxVolume << "\n";
    output << "fullscreen " << (settings.fullscreen ? 1 : 0) << "\n";
    output << "window_width " << settings.windowWidth << "\n";
    output << "window_height " << settings.windowHeight << "\n";
    output << "screen_shake " << (settings.screenShake ? 1 : 0) << "\n";
    output << "coop " << (settings.coop ? 1 : 0) << "\n";
    output << "aim_sensitivity " << settings.aimSensitivity << "\n";
    output << "key_up " << settings.keys.up << "\n";
    output << "key_down " << settings.keys.down << "\n";
    output << "key_left " << settings.keys.left << "\n";
    output << "key_right " << settings.keys.right << "\n";
    output << "key_reload " << settings.keys.reload << "\n";
    output << "key_pause " << settings.keys.pause << "\n";
    return true;
}

const char *KeyName(int key)
{
    static char buffer[16];

    if (key >= KEY_A && key <= KEY_Z)
    {
        buffer[0] = static_cast<char>('A' + (key - KEY_A));
        buffer[1] = '\0';
        return buffer;
    }
    if (key >= KEY_ZERO && key <= KEY_NINE)
    {
        buffer[0] = static_cast<char>('0' + (key - KEY_ZERO));
        buffer[1] = '\0';
        return buffer;
    }

    switch (key)
    {
    case KEY_ENTER: return "ENTER";
    case KEY_KP_ENTER: return "KP ENTER";
    case KEY_SPACE: return "SPACE";
    case KEY_TAB: return "TAB";
    case KEY_BACKSPACE: return "BKSP";
    case KEY_LEFT: return "LEFT";
    case KEY_RIGHT: return "RIGHT";
    case KEY_UP: return "UP";
    case KEY_DOWN: return "DOWN";
    case KEY_MINUS: return "-";
    case KEY_EQUAL: return "=";
    case KEY_COMMA: return ",";
    case KEY_PERIOD: return ".";
    case KEY_SLASH: return "/";
    case KEY_SEMICOLON: return ";";
    case KEY_APOSTROPHE: return "'";
    case KEY_LEFT_BRACKET: return "[";
    case KEY_RIGHT_BRACKET: return "]";
    case KEY_BACKSLASH: return "\\";
    case KEY_GRAVE: return "`";
    case KEY_ESCAPE: return "ESC";
    default: break;
    }

    std::snprintf(buffer, sizeof(buffer), "KEY %d", key);
    return buffer;
}
