#include "Assets.h"

#include <cstdio>
#include <string>

namespace
{
bool HasAssetTree(const std::string &root)
{
    const std::string probe = root + "/graphics/player.png";
    return FileExists(probe.c_str());
}

std::string WithSlash(std::string path)
{
    if (path.empty() || path.back() == '/' || path.back() == '\\')
        return path;
    return path + "/";
}
} // namespace

std::string FindAssetRoot()
{
    // Running from the build tree (assets copied next to the exe), then from
    // the source root, then relative to the executable itself.
    const char *candidates[] = {
        "assets",
        "../assets",
        "../../assets",
    };

    for (const char *candidate : candidates)
    {
        if (HasAssetTree(candidate))
            return candidate;
    }

    const std::string appDir = WithSlash(GetApplicationDirectory());
    const std::string appAssets = appDir + "assets";
    if (HasAssetTree(appAssets))
        return appAssets;

    return "assets";
}

std::string AssetPath(const char *relativePath)
{
    static const std::string root = FindAssetRoot();
    return root + "/" + relativePath;
}

std::string Assets::Path(const char *relativePath) const
{
    return root_ + "/" + relativePath;
}

bool Assets::LoadTextureFile(Texture2D &texture, const char *file)
{
    const std::string path = Path(file);
    texture = LoadTexture(path.c_str());
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0)
    {
        std::fprintf(stderr, "[assets] failed to load texture: %s\n", path.c_str());
        return false;
    }
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return true;
}

bool Assets::LoadSoundFile(Sound &sound, const char *file)
{
    if (!audioReady)
        return true;
    const std::string path = Path(file);
    sound = LoadSound(path.c_str());
    if (sound.frameCount <= 0)
    {
        std::fprintf(stderr, "[assets] failed to load sound: %s\n", path.c_str());
        return false;
    }
    return true;
}

bool Assets::LoadMusicFile(Music &music, const char *file)
{
    if (!audioReady)
        return true;
    const std::string path = Path(file);
    music = LoadMusicStream(path.c_str());
    if (music.frameCount <= 0 || music.ctxData == nullptr)
    {
        std::fprintf(stderr, "[assets] failed to load music: %s\n", path.c_str());
        return false;
    }
    return true;
}

bool Assets::Load()
{
    if (loaded_)
        return ok;

    root_ = FindAssetRoot();

    InitAudioDevice();
    audioReady = IsAudioDeviceReady();
    if (!audioReady)
        std::fprintf(stderr, "[assets] audio device unavailable, continuing muted\n");

    bool good = true;

    good &= LoadTextureFile(player, "graphics/player.png");
    good &= LoadTextureFile(bloater, "graphics/bloater.png");
    good &= LoadTextureFile(chaser, "graphics/chaser.png");
    good &= LoadTextureFile(crawler, "graphics/crawler.png");
    good &= LoadTextureFile(blood, "graphics/blood.png");
    good &= LoadTextureFile(crosshair, "graphics/crosshair.png");
    good &= LoadTextureFile(ammoIcon, "graphics/ammo_icon.png");
    good &= LoadTextureFile(ammoPickup, "graphics/ammo_pickup.png");
    good &= LoadTextureFile(healthPickup, "graphics/health_pickup.png");
    good &= LoadTextureFile(tilesheet, "graphics/background_sheet.png");
    good &= LoadTextureFile(menuBackground, "graphics/background3.png");

    const std::string fontPath = Path("fonts/big_noodle_titling.ttf");
    font = LoadFontEx(fontPath.c_str(), 150, nullptr, 0);
    if (font.texture.id == 0 || font.glyphCount == 0)
    {
        std::fprintf(stderr, "[assets] failed to load font: %s\n", fontPath.c_str());
        font = GetFontDefault();
        good = false;
    }
    else
    {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }

    good &= LoadSoundFile(hit, "sound/hit.wav");
    good &= LoadSoundFile(splat, "sound/splat.wav");
    good &= LoadSoundFile(death, "sound/deathscream.wav");
    good &= LoadSoundFile(shoot, "sound/shoot.wav");
    good &= LoadSoundFile(reload, "sound/reload.wav");
    good &= LoadSoundFile(reloadFailed, "sound/reload_failed.wav");
    good &= LoadSoundFile(powerUp, "sound/powerup.wav");
    good &= LoadSoundFile(pickup, "sound/pickup.wav");

    good &= LoadMusicFile(menuMusic, "sound/music.wav");
    good &= LoadMusicFile(gameMusic, "sound/gamemusic.wav");
    good &= LoadMusicFile(levelUpMusic, "sound/levelupmusic.wav");

    loaded_ = true;
    ok = good;
    return ok;
}

void Assets::Unload()
{
    if (!loaded_)
        return;

    UnloadTexture(player);
    UnloadTexture(bloater);
    UnloadTexture(chaser);
    UnloadTexture(crawler);
    UnloadTexture(blood);
    UnloadTexture(crosshair);
    UnloadTexture(ammoIcon);
    UnloadTexture(ammoPickup);
    UnloadTexture(healthPickup);
    UnloadTexture(tilesheet);
    UnloadTexture(menuBackground);

    if (font.texture.id != GetFontDefault().texture.id)
        UnloadFont(font);

    UnloadSound(hit);
    UnloadSound(splat);
    UnloadSound(death);
    UnloadSound(shoot);
    UnloadSound(reload);
    UnloadSound(reloadFailed);
    UnloadSound(powerUp);
    UnloadSound(pickup);

    UnloadMusicStream(menuMusic);
    UnloadMusicStream(gameMusic);
    UnloadMusicStream(levelUpMusic);

    if (audioReady)
        CloseAudioDevice();

    loaded_ = false;
    ok = false;
    audioReady = false;
}
