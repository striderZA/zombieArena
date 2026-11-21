#pragma once

#include "raylib.h"

#include <string>

// Locates the assets/ directory relative to the executable or the source tree.
std::string FindAssetRoot();
std::string AssetPath(const char *relativePath);

// Owns every GPU/audio resource used by the game. Cleanup happens once in
// Unload(), so gameplay classes can copy raylib handles freely.
class Assets
{
public:
    bool Load();
    void Unload();

    const std::string &Root() const { return root_; }
    std::string Path(const char *relativePath) const;

    Texture2D player{};
    Texture2D bloater{};
    Texture2D chaser{};
    Texture2D crawler{};
    Texture2D blood{};
    Texture2D crosshair{};
    Texture2D ammoIcon{};
    Texture2D ammoPickup{};
    Texture2D healthPickup{};
    Texture2D tilesheet{};
    Texture2D menuBackground{};

    Font font{};

    Sound hit{};
    Sound splat{};
    Sound death{};
    Sound shoot{};
    Sound reload{};
    Sound reloadFailed{};
    Sound powerUp{};
    Sound pickup{};

    Music menuMusic{};
    Music gameMusic{};
    Music levelUpMusic{};

    bool audioReady = false;
    bool ok = false;

private:
    bool LoadTextureFile(Texture2D &texture, const char *file);
    bool LoadSoundFile(Sound &sound, const char *file);
    bool LoadMusicFile(Music &music, const char *file);

    std::string root_;
    bool loaded_ = false;
};
