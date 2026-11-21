#include "Game.h"

#include "SweptCollision.h"
#include "Waves.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>

namespace
{
constexpr int kMaxBullets = 100;
constexpr float kPlayerHitShake = 0.45f;
constexpr float kDamageFlashTime = 0.40f;
constexpr int kMaxBloodDecals = 120;
constexpr int kMaxDamageNumbers = 80;
constexpr float kEnemyShotSpeed = 280.0f;
constexpr float kEnemyShotDamage = 10.0f;
constexpr int kEnemyShotRadius = 7;
constexpr int kSettingsRows = 9;
constexpr int kControlRows = 7;
constexpr int kBunkerRows = 7;

const char *kControlNames[6] = {"MOVE UP", "MOVE DOWN", "MOVE LEFT", "MOVE RIGHT", "RELOAD", "PAUSE"};

constexpr int kWindowSizes[3][2] = {
    {1280, 720},
    {1600, 900},
    {1920, 1080},
};

void CycleWindowSize(Settings &settings, int direction)
{
    int current = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (kWindowSizes[i][0] == settings.windowWidth && kWindowSizes[i][1] == settings.windowHeight)
        {
            current = i;
            break;
        }
    }

    current = (current + (direction > 0 ? 1 : 2)) % 3;
    settings.windowWidth = kWindowSizes[current][0];
    settings.windowHeight = kWindowSizes[current][1];

    if (!IsWindowFullscreen())
        SetWindowSize(settings.windowWidth, settings.windowHeight);
}

void PlaySoundSafe(Sound &sound, bool audioReady)
{
    if (audioReady)
        PlaySound(sound);
}

Color RarityColor(UpgradeRarity rarity)
{
    switch (rarity)
    {
    case UpgradeRarity::Common:
        return Color{230, 230, 230, 255};
    case UpgradeRarity::Rare:
        return Color{90, 200, 255, 255};
    case UpgradeRarity::Epic:
        return Color{205, 110, 255, 255};
    }
    return WHITE;
}

float Distance(Vector2 a, Vector2 b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
} // namespace

bool Game::Init(const GameOptions &options)
{
    options_ = options;
    settings_ = options.settings;

    if (!assets_.Load())
        return false;

    profilePath_ = options_.profilePath.empty() ? assets_.Path("gamedata/profile.txt")
                                                : options_.profilePath;
    settingsPath_ = options_.settingsPath.empty() ? assets_.Path("gamedata/settings.txt")
                                                  : options_.settingsPath;
    LoadProfile(profilePath_, profile_);

    for (PlayerSlot &slot : players_)
    {
        slot.player.SetTexture(assets_.player);
        slot.arsenal.Reset();
    }

    healthPickup_.SetTexture(assets_.healthPickup);
    ammoPickup_.SetTexture(assets_.ammoPickup);

    playerCount_ = (settings_.coop && GamepadPresent()) ? 2 : 1;

    ApplySettings();
    LoadHiScore();

    camera_.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera_.target = {0.0f, 0.0f};
    camera_.rotation = 0.0f;
    camera_.zoom = 1.0f;

    state_ = GameState::Title;
    PlayMusic(assets_.menuMusic);

    initialized_ = true;

    if (options_.startWave > 0)
        NewRun();

    return true;
}

void Game::Shutdown()
{
    if (!initialized_)
        return;

    if (runActive_)
        FinishRun();

    SaveHiScore();
    SaveSettings();
    StopMusic();
    assets_.Unload();
    ShowCursor();
    initialized_ = false;
}

bool Game::GamepadPresent() const
{
    return options_.gamepadAvailable || IsGamepadAvailable(0);
}

void Game::LoadHiScore()
{
    std::ifstream input(assets_.Path("gamedata/scores.txt"));
    if (input.is_open())
        input >> hiScore_;
    if (hiScore_ < 0)
        hiScore_ = 0;
}

void Game::SaveHiScore() const
{
    std::ofstream output(assets_.Path("gamedata/scores.txt"));
    if (output.is_open())
        output << hiScore_ << "\n";
}

void Game::PlayMusic(Music &music)
{
    if (!assets_.audioReady)
        return;
    if (currentMusic_ == &music)
        return;

    StopMusic();
    currentMusic_ = &music;
    SetMusicVolume(music, settings_.musicVolume);
    PlayMusicStream(music);
    SeekMusicStream(music, 0.0f);
}

void Game::StopMusic()
{
    if (currentMusic_ != nullptr && assets_.audioReady)
        StopMusicStream(*currentMusic_);
    currentMusic_ = nullptr;
}

void Game::ApplySettings()
{
    if (!assets_.audioReady)
        return;

    SetMasterVolume(settings_.masterVolume);
    if (currentMusic_ != nullptr)
        SetMusicVolume(*currentMusic_, settings_.musicVolume);

    const float sfx = settings_.sfxVolume;
    SetSoundVolume(assets_.hit, sfx);
    SetSoundVolume(assets_.splat, sfx);
    SetSoundVolume(assets_.death, sfx);
    SetSoundVolume(assets_.shoot, sfx);
    SetSoundVolume(assets_.reload, sfx);
    SetSoundVolume(assets_.reloadFailed, sfx);
    SetSoundVolume(assets_.powerUp, sfx);
    SetSoundVolume(assets_.pickup, sfx);

    if (!settings_.screenShake)
        shakeTrauma_ = 0.0f;
}

void Game::SaveSettings()
{
    if (!settingsPath_.empty())
        ::SaveSettings(settingsPath_, settings_);
}

void Game::Update(float dt, const InputState &input)
{
    if (!initialized_)
        return;

    gameTime_ += dt;

    if (assets_.audioReady && currentMusic_ != nullptr)
        UpdateMusicStream(*currentMusic_);

    switch (state_)
    {
    case GameState::Title:
        if (input.toggleCoop && GamepadPresent())
        {
            settings_.coop = !settings_.coop;
            SaveSettings();
            PlaySoundSafe(assets_.powerUp, assets_.audioReady);
        }
        if (input.openBunker)
        {
            OpenBunker();
            break;
        }
        if (input.openSettings)
        {
            OpenSettings(GameState::Title);
            break;
        }
        if (input.confirm)
            NewRun();
        break;

    case GameState::GameOver:
        if (input.openBunker)
        {
            OpenBunker();
            break;
        }
        if (input.openSettings)
        {
            OpenSettings(GameState::GameOver);
            break;
        }
        if (input.confirm)
            NewRun();
        break;

    case GameState::Paused:
        if (input.pausePressed || input.confirm)
        {
            state_ = GameState::Playing;
            break;
        }
        if (input.openSettings)
        {
            OpenSettings(GameState::Paused);
            break;
        }
        break;

    case GameState::LevelingUp:
        if (input.upgrade >= 1 && input.upgrade <= draftSize_)
        {
            ApplyUpgrade(input.upgrade - 1);
            BeginWave(wave_ + 1);
        }
        break;

    case GameState::Playing:
        if (input.pausePressed)
        {
            state_ = GameState::Paused;
            shakeTrauma_ = 0.0f;
            damageFlash_ = 0.0f;
            for (PlayerSlot &slot : players_)
                slot.muzzleFlash = 0.0f;
            break;
        }
        UpdatePlaying(dt, input);
        break;

    case GameState::Bunker:
        UpdateBunker(input);
        break;

    case GameState::Settings:
        UpdateSettings(input);
        break;
    }
}

void Game::NewRun()
{
    wave_ = 0;
    score_ = 0;
    currentBullet_ = 0;
    runKills_ = 0;
    scrapRun_ = 0;
    runActive_ = false;

    for (Bullet &bullet : bullets_)
        bullet.Stop();

    healthPickup_.Reset();
    ammoPickup_.Reset();
    upgradeStacks_.fill(0);

    runSeed_ = options_.seed ^ (static_cast<std::uint64_t>(++runCounter_) * 0x9E3779B97F4A7C15ull);
    combatRng_ = Rng(runSeed_ ^ 0x51ED270F71A7B00Dull);

    playerCount_ = (settings_.coop && GamepadPresent()) ? 2 : 1;
    scrapMultiplier_ = 1.0f + 0.2f * static_cast<float>(profile_.levels[kMetaMagnet]);
    ammoMetaMultiplier_ = 1.0f + 0.1f * static_cast<float>(profile_.levels[kMetaPockets]);

    for (int i = 0; i < 2; ++i)
    {
        PlayerSlot &slot = players_[i];
        slot.player.ResetStats();
        slot.arsenal.Reset();
        slot.lastShotTime = -1000.0;
        slot.aimDirection = {1.0f, 0.0f};
        slot.crosshairScreen = camera_.offset;
        slot.muzzleFlash = 0.0f;

        if (i < playerCount_)
            ApplyMetaToSlot(slot);
    }

    bloodDecals_.clear();
    damageNumbers_.clear();
    enemyShots_.clear();
    shockwaves_.clear();
    shakeTrauma_ = 0.0f;
    damageFlash_ = 0.0f;
    bannerTimer_ = 0.0f;
    bannerText_.clear();

    if (options_.startWave > 0)
    {
        BeginWave(options_.startWave);
        return;
    }

    GenerateDraft();
    state_ = GameState::LevelingUp;
    PlayMusic(assets_.levelUpMusic);
}

void Game::ApplyMetaToSlot(PlayerSlot &slot)
{
    const int vest = profile_.levels[kMetaVest];
    if (vest > 0)
        slot.player.AddMaxHealth(5.0f * static_cast<float>(vest), true);

    const int shoes = profile_.levels[kMetaShoes];
    if (shoes > 0)
        slot.player.MultiplySpeed(std::pow(1.03f, static_cast<float>(shoes)));

    const int tuning = profile_.levels[kMetaTuning];
    if (tuning > 0)
        slot.arsenal.AddDamage(tuning);

    slot.arsenal.SetUnlocked(WeaponType::Smg, profile_.smgUnlocked);
}

void Game::FinishRun()
{
    if (!runActive_)
        return;

    profile_.scrap += scrapRun_;
    profile_.totalScrap += scrapRun_;
    profile_.runs += 1;
    profile_.kills += runKills_;
    profile_.bestWave = std::max(profile_.bestWave, wave_);
    SaveProfile(profilePath_, profile_);
    runActive_ = false;
}

void Game::AwardScrap(int baseAmount)
{
    if (baseAmount <= 0)
        return;

    const int amount = static_cast<int>(std::lround(static_cast<float>(baseAmount) * scrapMultiplier_));
    scrapRun_ += std::max(1, amount);
}

void Game::GenerateDraft()
{
    draft_.fill(-1);
    draftSize_ = 0;

    int pool[kUpgradeCount];
    int poolSize = 0;
    for (int id = 0; id < kUpgradeCount; ++id)
    {
        if (id == kRepairKit)
            continue;
        const UpgradeDef &def = UpgradeDefinition(id);
        if (def.weight > 0 && upgradeStacks_[id] < def.maxStacks)
            pool[poolSize++] = id;
    }

    while (draftSize_ < kDraftSize && poolSize > 0)
    {
        int totalWeight = 0;
        for (int i = 0; i < poolSize; ++i)
            totalWeight += UpgradeDefinition(pool[i]).weight;

        int pick = 0;
        if (totalWeight > 0)
        {
            const int roll = combatRng_.RangeInt(0, totalWeight);
            int accumulated = 0;
            for (int i = 0; i < poolSize; ++i)
            {
                accumulated += UpgradeDefinition(pool[i]).weight;
                if (roll < accumulated)
                {
                    pick = i;
                    break;
                }
            }
        }

        draft_[draftSize_++] = pool[pick];
        pool[pick] = pool[--poolSize];
    }

    if (draftSize_ < kDraftSize)
        draft_[draftSize_++] = kRepairKit;
}

void Game::ApplyUpgrade(int slot)
{
    if (slot < 0 || slot >= draftSize_)
        return;

    const int id = draft_[slot];
    if (id < 0 || id >= kUpgradeCount)
        return;

    const UpgradeDef &def = UpgradeDefinition(id);
    if (upgradeStacks_[id] < def.maxStacks)
        ++upgradeStacks_[id];

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &playerSlot = players_[i];
        switch (id)
        {
        case kRapidFire:
            playerSlot.arsenal.MultiplyFireRate(1.20f);
            break;
        case kExtendedMag:
            playerSlot.arsenal.ApplyClipUpgrade();
            break;
        case kVitality:
            playerSlot.player.AddMaxHealth(10.0f, playerSlot.player.Alive());
            break;
        case kSwift:
            playerSlot.player.UpgradeSpeed();
            break;
        case kHollowPoint:
            playerSlot.arsenal.AddDamage(1);
            break;
        case kSteadyHands:
            playerSlot.arsenal.MultiplySpread(0.80f);
            break;
        case kShotgunChoke:
            playerSlot.arsenal.AddExtraPellet();
            break;
        case kAdrenaline:
            playerSlot.arsenal.AddHealPerKill(4);
            break;
        case kBulletStorm:
            playerSlot.arsenal.AddExtraProjectile();
            break;
        case kRepairKit:
            if (playerSlot.player.Alive())
                playerSlot.player.Heal(25.0f);
            break;
        default:
            break;
        }
    }

    if (id == kFieldMedic)
        healthPickup_.Upgrade();
    if (id == kScavenger)
        ammoPickup_.Upgrade();

    draft_.fill(-1);
    draftSize_ = 0;
}

void Game::BeginWave(int wave)
{
    wave_ = wave;
    arena_.SetWave(wave_, runSeed_);

    healthPickup_.SetObstacles(&arena_.Obstacles());
    ammoPickup_.SetObstacles(&arena_.Obstacles());

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        const Vector2 offset = (playerCount_ > 1) ? (i == 0 ? Vector2{-40.0f, 0.0f} : Vector2{40.0f, 0.0f})
                                                  : Vector2{0.0f, 0.0f};
        slot.player.Spawn(arena_.Bounds(), static_cast<float>(arena_.TileSize()), offset);
        slot.player.Revive(0.5f);
        slot.lastShotTime = -1000.0;
        slot.muzzleFlash = 0.0f;
    }

    UpdateCamera();

    healthPickup_.SetArena(arena_.Bounds(), pickupRng_);
    ammoPickup_.SetArena(arena_.Bounds(), pickupRng_);

    spawnRng_ = Rng(runSeed_ ^ (static_cast<std::uint64_t>(wave_) * 0x9E3779B97F4A7C15ull));

    const std::vector<ZombieType> composition = ComposeWave(wave_, playerCount_ > 1, spawnRng_);
    totalZombies_ = static_cast<int>(composition.size());
    zombies_.clear();
    zombies_.reserve(composition.size());

    const float minX = 20.0f;
    const float minY = 20.0f;
    const float maxX = arena_.Size() - 20.0f;
    const float maxY = arena_.Size() - 20.0f;

    bool eliteSpawned = false;
    for (std::size_t index = 0; index < composition.size(); ++index)
    {
        const ZombieType type = composition[index];

        // From wave 4 every wave includes at least one elite; later waves add
        // random elites on top, up to a 25% chance per zombie.
        bool elite = false;
        if (type != ZombieType::Boss && wave_ >= 4)
        {
            if (!eliteSpawned)
            {
                elite = true;
                eliteSpawned = true;
            }
            else
            {
                const float chance = std::min(0.05f + 0.015f * static_cast<float>(wave_), 0.25f);
                elite = spawnRng_.Range(0.0f, 1.0f) < chance;
            }
        }

        const int side = spawnRng_.RangeInt(0, 4);
        float x = 0.0f;
        float y = 0.0f;

        switch (side)
        {
        case 0:
            x = minX;
            y = spawnRng_.Range(minY, maxY);
            break;
        case 1:
            x = maxX;
            y = spawnRng_.Range(minY, maxY);
            break;
        case 2:
            x = spawnRng_.Range(minX, maxX);
            y = minY;
            break;
        default:
            x = spawnRng_.Range(minX, maxX);
            y = maxY;
            break;
        }

        const float variance = spawnRng_.Range(0.91f, 1.0f);
        const float speed = Zombie::BaseSpeed(type) * variance +
                            Zombie::WaveSpeedBonus(type) * 0.1f * static_cast<float>(wave_);
        const int health = Zombie::BaseHealth(type) + (type == ZombieType::Boss ? wave_ * 8 : 0);

        Zombie zombie;
        zombie.Spawn(type, {x, y}, ZombieTexture(type), assets_.blood, speed, health, elite);
        zombies_.push_back(zombie);
    }

    zombiesAlive_ = totalZombies_;
    state_ = GameState::Playing;
    runActive_ = true;

    bloodDecals_.clear();
    damageNumbers_.clear();
    enemyShots_.clear();
    shockwaves_.clear();
    bannerText_ = "WAVE " + std::to_string(wave_);
    bannerTimer_ = 2.4f;
    AddShake(0.12f);

    PlayMusic(assets_.gameMusic);
    PlaySoundSafe(assets_.powerUp, assets_.audioReady);
}

Texture2D Game::ZombieTexture(ZombieType type) const
{
    switch (type)
    {
    case ZombieType::Spitter:
    case ZombieType::Sprinter:
        return assets_.chaser;
    case ZombieType::Exploder:
    case ZombieType::Boss:
        return assets_.bloater;
    case ZombieType::Bloater:
        return assets_.bloater;
    case ZombieType::Chaser:
        return assets_.chaser;
    case ZombieType::Crawler:
        return assets_.crawler;
    }
    return assets_.chaser;
}

void Game::HandleWeaponInput(int index, const PlayerInput &input)
{
    PlayerSlot &slot = players_[index];
    if (input.weaponSelect >= 1 && input.weaponSelect <= kWeaponCount)
        slot.arsenal.Switch(static_cast<WeaponType>(input.weaponSelect - 1));
    if (input.weaponCycle != 0)
        slot.arsenal.Cycle(input.weaponCycle);
}

void Game::HandleReload(int index)
{
    PlayerSlot &slot = players_[index];
    const WeaponType type = slot.arsenal.Current();
    WeaponState &state = slot.arsenal.CurrentState();
    const int maxClip = slot.arsenal.EffectiveClip(type);

    if (state.clip >= maxClip)
        return;

    if (state.spare >= maxClip)
    {
        state.clip = maxClip;
        state.spare -= maxClip;
        PlaySoundSafe(assets_.reload, assets_.audioReady);
    }
    else if (state.spare > 0)
    {
        state.clip = state.spare;
        state.spare = 0;
        PlaySoundSafe(assets_.reload, assets_.audioReady);
    }
    else
    {
        PlaySoundSafe(assets_.reloadFailed, assets_.audioReady);
    }
}

void Game::FireWeapon(int index, Vector2 target)
{
    PlayerSlot &slot = players_[index];
    const WeaponType type = slot.arsenal.Current();
    WeaponState &state = slot.arsenal.CurrentState();
    if (state.clip <= 0)
        return;

    --state.clip;

    const int projectiles = slot.arsenal.ProjectilesForShot(type);
    const float spread = WeaponDefinition(type).spread * slot.arsenal.SpreadMultiplier();
    const int damage = slot.arsenal.DamagePerProjectile();

    for (int i = 0; i < projectiles; ++i)
    {
        bullets_[currentBullet_].Shoot(slot.player.Center(), target, spread, combatRng_, damage);
        currentBullet_ = (currentBullet_ + 1) % kMaxBullets;
    }

    slot.lastShotTime = gameTime_;
    slot.muzzleFlash = 0.07f;
    AddShake(type == WeaponType::Shotgun ? 0.12f : 0.05f);
    PlaySoundSafe(assets_.shoot, assets_.audioReady);
}

Vector2 Game::AimTarget(int index, const PlayerInput &input)
{
    PlayerSlot &slot = players_[index];
    if (input.useStickAim)
    {
        const float length = std::sqrt(input.stickAim.x * input.stickAim.x +
                                       input.stickAim.y * input.stickAim.y);
        if (length > 0.25f)
            slot.aimDirection = {input.stickAim.x / length, input.stickAim.y / length};

        const float reach = 420.0f * settings_.aimSensitivity;
        return {slot.player.Center().x + slot.aimDirection.x * reach,
                slot.player.Center().y + slot.aimDirection.y * reach};
    }

    return ScreenToWorld(input.mouseScreen);
}

void Game::UpdatePlayer(int index, float dt, const PlayerInput &input)
{
    PlayerSlot &slot = players_[index];
    const Vector2 aim = AimTarget(index, input);
    slot.crosshairScreen = input.useStickAim ? WorldToScreen(aim, camera_) : input.mouseScreen;

    if (!slot.player.Alive())
        return;

    slot.player.Update(dt, input.move, aim, &arena_.Obstacles());
    HandleWeaponInput(index, input);

    if (input.reload)
        HandleReload(index);

    const WeaponType type = slot.arsenal.Current();
    const float rate = slot.arsenal.EffectiveFireRate(type);
    if (input.fire && slot.arsenal.CurrentState().clip > 0 && rate > 0.0f &&
        gameTime_ - slot.lastShotTime >= 1.0 / static_cast<double>(rate))
    {
        FireWeapon(index, aim);
    }
}

void Game::UpdateCamera()
{
    Vector2 focus{0.0f, 0.0f};
    int alive = 0;

    for (int i = 0; i < playerCount_; ++i)
    {
        if (!players_[i].player.Alive())
            continue;
        focus.x += players_[i].player.Center().x;
        focus.y += players_[i].player.Center().y;
        ++alive;
    }

    if (alive > 0)
        camera_.target = {focus.x / static_cast<float>(alive), focus.y / static_cast<float>(alive)};

    camera_.zoom = 1.0f;
    if (playerCount_ > 1 && players_[0].player.Alive() && players_[1].player.Alive())
    {
        const Vector2 a = players_[0].player.Center();
        const Vector2 b = players_[1].player.Center();
        const float dx = std::fabs(a.x - b.x);
        const float dy = std::fabs(a.y - b.y);
        const float zoomX = (static_cast<float>(GetScreenWidth()) - 160.0f) / (dx + 320.0f);
        const float zoomY = (static_cast<float>(GetScreenHeight()) - 160.0f) / (dy + 320.0f);
        camera_.zoom = std::clamp(std::min({1.0f, zoomX, zoomY}), 0.55f, 1.0f);
    }
}

void Game::NearestPlayerTarget(Vector2 from, Vector2 &position, float &distance) const
{
    bool found = false;
    float best = 0.0f;
    Vector2 bestPosition = players_[0].player.Center();

    for (int i = 0; i < playerCount_; ++i)
    {
        if (!players_[i].player.Alive())
            continue;

        const Vector2 candidate = players_[i].player.Center();
        const float candidateDistance = Distance(from, candidate);
        if (!found || candidateDistance < best)
        {
            found = true;
            best = candidateDistance;
            bestPosition = candidate;
        }
    }

    position = bestPosition;
    distance = found ? best : 0.0f;
}

void Game::SpawnEnemyShot(Vector2 from, Vector2 target)
{
    const float dx = target.x - from.x;
    const float dy = target.y - from.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.001f)
        return;

    EnemyShot shot;
    shot.position = from;
    shot.velocity = {dx / length * kEnemyShotSpeed, dy / length * kEnemyShotSpeed};
    shot.life = 4.0f;
    shot.damage = static_cast<int>(kEnemyShotDamage);
    enemyShots_.push_back(shot);
}

void Game::UpdatePlaying(float dt, const InputState &input)
{
    for (int i = 0; i < playerCount_; ++i)
        UpdatePlayer(i, dt, input.players[i]);

    UpdateCamera();

    for (Zombie &zombie : zombies_)
    {
        Vector2 target;
        float distance = 0.0f;
        NearestPlayerTarget(zombie.Position(), target, distance);

        zombie.Update(dt, target, &arena_.Obstacles());

        if (zombie.Alive() && zombie.Ranged() && distance < 520.0f && zombie.TickAttack(dt))
            SpawnEnemyShot(zombie.Position(), target);
    }

    UpdateEnemyShots(dt);

    healthPickup_.Update(dt, pickupRng_);
    ammoPickup_.Update(dt, pickupRng_);

    UpdateBullets(dt);
    UpdateCollisions();
    UpdateJuice(dt);

    if (state_ == GameState::Playing && AllPlayersDead())
        EnterGameOver();
}

void Game::UpdateEnemyShots(float dt)
{
    std::vector<EnemyShot> survivors;
    survivors.reserve(enemyShots_.size());

    for (const EnemyShot &shot : enemyShots_)
    {
        EnemyShot updated = shot;
        updated.position.x += updated.velocity.x * dt;
        updated.position.y += updated.velocity.y * dt;
        updated.life -= dt;

        bool consumed = updated.life <= 0.0f;
        if (!consumed)
        {
            const Rectangle arena = arena_.Bounds();
            if (updated.position.x < arena.x - 120.0f || updated.position.x > arena.x + arena.width + 120.0f ||
                updated.position.y < arena.y - 120.0f || updated.position.y > arena.y + arena.height + 120.0f)
            {
                consumed = true;
            }
        }

        if (!consumed)
        {
            for (const Rectangle &rect : arena_.Obstacles())
            {
                if (CheckCollisionCircleRec(updated.position, static_cast<float>(kEnemyShotRadius), rect))
                {
                    consumed = true;
                    AddShockwave(updated.position, 10.0f, 0.16f, LIGHTGRAY);
                    break;
                }
            }
        }

        if (!consumed)
        {
            for (int i = 0; i < playerCount_; ++i)
            {
                PlayerSlot &slot = players_[i];
                if (!slot.player.Alive())
                    continue;
                if (!CheckCollisionCircleRec(updated.position, static_cast<float>(kEnemyShotRadius),
                                             slot.player.Rect()))
                    continue;

                consumed = true;
                if (slot.player.TakeHit(gameTime_, static_cast<float>(updated.damage)))
                {
                    PlaySoundSafe(assets_.hit, assets_.audioReady);
                    AddShake(0.35f);
                    damageFlash_ = kDamageFlashTime;
                }
                if (!slot.player.Alive())
                    AddBloodDecal(slot.player.Center(), 1.2f, 8.0f);
                break;
            }
        }

        if (!consumed)
            survivors.push_back(updated);
    }

    enemyShots_.swap(survivors);

    if (state_ == GameState::Playing && AllPlayersDead())
        EnterGameOver();
}

void Game::UpdateBullets(float dt)
{
    for (Bullet &bullet : bullets_)
        bullet.Update(dt);
}

void Game::UpdateCollisions()
{
    for (Bullet &bullet : bullets_)
    {
        if (!bullet.InFlight())
            continue;

        // A 0.1 s frame moves a bullet 100 px, which is wider than a zombie.
        // Test the whole segment from last frame's position to this one and
        // resolve the nearest zombie along that path.
        Zombie *hit = nullptr;
        float nearestT = 2.0f;
        for (Zombie &zombie : zombies_)
        {
            if (!zombie.Alive())
                continue;

            const Rectangle rect = zombie.Rect();
            const Rectangle expanded{
                rect.x - Bullet::kWidth * 0.5f,
                rect.y - Bullet::kHeight * 0.5f,
                rect.width + Bullet::kWidth,
                rect.height + Bullet::kHeight};

            float t = 0.0f;
            if (SegmentIntersectsRect(bullet.PreviousPosition(), bullet.Position(), expanded, &t) &&
                t < nearestT)
            {
                nearestT = t;
                hit = &zombie;
            }
        }

        // Cover blocks the shot at the nearest impact point along the path.
        // A wall in front of a zombie wins the hit resolution.
        float wallT = 2.0f;
        for (const Rectangle &rect : arena_.Obstacles())
        {
            float t = 0.0f;
            if (SegmentIntersectsRect(bullet.PreviousPosition(), bullet.Position(), rect, &t) && t < wallT)
                wallT = t;
        }

        if (wallT < 2.0f && wallT <= nearestT)
        {
            bullet.Stop();
            const Vector2 impact{
                bullet.PreviousPosition().x + (bullet.Position().x - bullet.PreviousPosition().x) * wallT,
                bullet.PreviousPosition().y + (bullet.Position().y - bullet.PreviousPosition().y) * wallT};
            AddShockwave(impact, 9.0f, 0.16f, LIGHTGRAY);
            continue;
        }

        if (hit == nullptr)
            continue;

        bullet.Stop();
        const int damage = bullet.Damage();
        const bool killed = hit->Hit(damage);

        hit->Knockback(bullet.Direction(), 5.0f + static_cast<float>(damage) * 2.0f);
        hit->ResolveObstacles(arena_.Obstacles());
        hit->SetFlash(0.08f);
        AddDamageNumber(hit->Position(), damage);
        if (killed || combatRng_.Range(0.0f, 1.0f) < 0.30f)
            AddBloodDecal(hit->Position(), killed ? 1.0f : 0.55f, killed ? 9.0f : 4.5f);
        AddShake(killed ? 0.07f : 0.02f);
        PlaySoundSafe(assets_.splat, assets_.audioReady);

        if (killed)
        {
            const ZombieType type = hit->Type();
            const bool elite = hit->Elite();
            score_ += Zombie::ScoreValue(type, elite);
            if (score_ > hiScore_)
                hiScore_ = score_;
            AwardScrap(Zombie::ScrapValue(type, elite));
            ++runKills_;

            for (int i = 0; i < playerCount_; ++i)
            {
                const int heal = players_[i].arsenal.HealPerKill();
                if (heal > 0)
                    players_[i].player.Heal(static_cast<float>(heal));
            }

            if (type == ZombieType::Exploder)
                ExplodeAt(hit->Position(), 110.0f, 20.0f);
            else if (type == ZombieType::Boss)
            {
                AddShockwave(hit->Position(), 190.0f, 0.7f, RED);
                AddShake(0.7f);
                healthPickup_.ForceSpawn(hit->Position());
                ammoPickup_.ForceSpawn(hit->Position());
            }

            if (elite)
            {
                AddShockwave(hit->Position(), 90.0f, 0.45f, GOLD);
                AddShake(0.25f);
                if (combatRng_.Range(0.0f, 1.0f) < 0.5f)
                    healthPickup_.ForceSpawn(hit->Position());
                else
                    ammoPickup_.ForceSpawn(hit->Position());
            }

            --zombiesAlive_;
            if (zombiesAlive_ <= 0)
                EnterLevelingUp();
        }

        if (state_ != GameState::Playing)
            return;
    }

    if (state_ != GameState::Playing)
        return;

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        if (!slot.player.Alive())
            continue;

        for (Zombie &zombie : zombies_)
        {
            if (!zombie.Alive())
                continue;
            if (!CheckCollisionRecs(slot.player.Rect(), zombie.Rect()))
                continue;

            if (slot.player.TakeHit(gameTime_, zombie.ContactDamage()))
            {
                PlaySoundSafe(assets_.hit, assets_.audioReady);
                AddShake(kPlayerHitShake);
                damageFlash_ = kDamageFlashTime;
            }

            if (!slot.player.Alive())
            {
                AddBloodDecal(slot.player.Center(), 1.3f, 9.0f);
                break;
            }
        }
    }

    if (AllPlayersDead())
    {
        EnterGameOver();
        return;
    }

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        if (!slot.player.Alive())
            continue;

        if (healthPickup_.IsSpawned() && CheckCollisionRecs(slot.player.Rect(), healthPickup_.Rect()))
        {
            slot.player.Heal(static_cast<float>(healthPickup_.GotIt()));
            PlaySoundSafe(assets_.pickup, assets_.audioReady);
            break;
        }

        if (ammoPickup_.IsSpawned() && CheckCollisionRecs(slot.player.Rect(), ammoPickup_.Rect()))
        {
            const int amount =
                static_cast<int>(std::lround(static_cast<float>(ammoPickup_.GotIt()) * ammoMetaMultiplier_));
            for (int w = 0; w < kWeaponCount; ++w)
            {
                const WeaponType type = static_cast<WeaponType>(w);
                if (slot.arsenal.IsUnlocked(type))
                    slot.arsenal.State(type).spare += slot.arsenal.AmmoPickupFor(type, amount);
            }
            PlaySoundSafe(assets_.reload, assets_.audioReady);
            break;
        }
    }
}

void Game::ExplodeAt(Vector2 position, float radius, float damage)
{
    AddShockwave(position, radius, 0.45f, ORANGE);
    AddShake(0.5f);
    PlaySoundSafe(assets_.hit, assets_.audioReady);

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        if (!slot.player.Alive())
            continue;
        if (Distance(position, slot.player.Center()) > radius)
            continue;

        if (slot.player.TakeHit(gameTime_, damage))
        {
            damageFlash_ = kDamageFlashTime;
            AddShake(kPlayerHitShake);
        }
        if (!slot.player.Alive())
            AddBloodDecal(slot.player.Center(), 1.3f, 9.0f);
    }

    if (AllPlayersDead())
        EnterGameOver();
}

void Game::AddShake(float amount)
{
    if (!settings_.screenShake)
        return;
    shakeTrauma_ = std::min(1.0f, shakeTrauma_ + amount);
}

void Game::AddBloodDecal(Vector2 position, float scale, float life)
{
    if (static_cast<int>(bloodDecals_.size()) >= kMaxBloodDecals)
        bloodDecals_.erase(bloodDecals_.begin());

    BloodDecal decal;
    decal.position = {
        position.x + combatRng_.Range(-7.0f, 7.0f),
        position.y + combatRng_.Range(-7.0f, 7.0f),
    };
    decal.rotation = combatRng_.Range(0.0f, 360.0f);
    decal.scale = scale * combatRng_.Range(0.85f, 1.15f);
    decal.life = life;
    bloodDecals_.push_back(decal);
}

void Game::AddDamageNumber(Vector2 position, int amount)
{
    if (static_cast<int>(damageNumbers_.size()) >= kMaxDamageNumbers)
        damageNumbers_.erase(damageNumbers_.begin());

    DamageNumber number;
    number.position = {
        position.x + combatRng_.Range(-9.0f, 9.0f),
        position.y - combatRng_.Range(30.0f, 46.0f),
    };
    number.life = 0.8f;
    number.amount = amount;
    damageNumbers_.push_back(number);
}

void Game::AddShockwave(Vector2 position, float radius, float life, Color color)
{
    Shockwave wave;
    wave.position = position;
    wave.radius = radius;
    wave.life = life;
    wave.maxLife = life;
    wave.color = color;
    shockwaves_.push_back(wave);
}

void Game::UpdateJuice(float dt)
{
    shakeTrauma_ = std::max(0.0f, shakeTrauma_ - dt * 2.0f);
    damageFlash_ = std::max(0.0f, damageFlash_ - dt);
    bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);

    for (PlayerSlot &slot : players_)
        slot.muzzleFlash = std::max(0.0f, slot.muzzleFlash - dt);

    for (BloodDecal &decal : bloodDecals_)
        decal.life -= dt;
    bloodDecals_.erase(
        std::remove_if(bloodDecals_.begin(), bloodDecals_.end(),
                       [](const BloodDecal &decal) { return decal.life <= 0.0f; }),
        bloodDecals_.end());

    for (DamageNumber &number : damageNumbers_)
    {
        number.life -= dt;
        number.position.y -= 40.0f * dt;
    }
    damageNumbers_.erase(
        std::remove_if(damageNumbers_.begin(), damageNumbers_.end(),
                       [](const DamageNumber &number) { return number.life <= 0.0f; }),
        damageNumbers_.end());

    for (Shockwave &wave : shockwaves_)
        wave.life -= dt;
    shockwaves_.erase(
        std::remove_if(shockwaves_.begin(), shockwaves_.end(),
                       [](const Shockwave &wave) { return wave.life <= 0.0f; }),
        shockwaves_.end());
}

void Game::EnterLevelingUp()
{
    AwardScrap(5 + wave_ * 2);
    state_ = GameState::LevelingUp;
    GenerateDraft();
    PlayMusic(assets_.levelUpMusic);
}

void Game::EnterGameOver()
{
    state_ = GameState::GameOver;
    PlaySoundSafe(assets_.death, assets_.audioReady);
    StopMusic();
    SaveHiScore();
    FinishRun();
}

bool Game::AllPlayersDead() const
{
    for (int i = 0; i < playerCount_; ++i)
    {
        if (players_[i].player.Alive())
            return false;
    }
    return true;
}

int Game::AlivePlayerCount() const
{
    int alive = 0;
    for (int i = 0; i < playerCount_; ++i)
    {
        if (players_[i].player.Alive())
            ++alive;
    }
    return alive;
}

int Game::EliteZombiesAlive() const
{
    int alive = 0;
    for (const Zombie &zombie : zombies_)
    {
        if (zombie.Alive() && zombie.Elite())
            ++alive;
    }
    return alive;
}

int Game::BulletsInFlight() const
{
    int count = 0;
    for (const Bullet &bullet : bullets_)
    {
        if (bullet.InFlight())
            ++count;
    }
    return count;
}

const Player &Game::GetPlayer(int index) const
{
    const int clamped = std::clamp(index, 0, 1);
    return players_[clamped].player;
}

float Game::PlayerHealth(int index) const
{
    return GetPlayer(index).Health();
}

int Game::CurrentWeapon(int playerIndex) const
{
    const int clamped = std::clamp(playerIndex, 0, 1);
    return static_cast<int>(players_[clamped].arsenal.Current());
}

int Game::WeaponClipFor(int playerIndex, int weapon) const
{
    if (playerIndex < 0 || playerIndex > 1 || weapon < 0 || weapon >= kWeaponCount)
        return 0;
    return players_[playerIndex].arsenal.State(static_cast<WeaponType>(weapon)).clip;
}

int Game::WeaponSpareFor(int playerIndex, int weapon) const
{
    if (playerIndex < 0 || playerIndex > 1 || weapon < 0 || weapon >= kWeaponCount)
        return 0;
    return players_[playerIndex].arsenal.State(static_cast<WeaponType>(weapon)).spare;
}

float Game::FireRateMultiplier() const
{
    return players_[0].arsenal.FireRateMultiplier();
}

int Game::DraftOption(int slot) const
{
    if (slot < 0 || slot >= draftSize_)
        return -1;
    return draft_[slot];
}

int Game::UpgradeStacks(int upgradeId) const
{
    if (upgradeId < 0 || upgradeId >= kUpgradeCount)
        return 0;
    return upgradeStacks_[upgradeId];
}

float Game::ArenaSize() const
{
    return arena_.Size();
}

int Game::EnemyShotsActive() const
{
    return static_cast<int>(enemyShots_.size());
}

bool Game::HasBoss() const
{
    for (const Zombie &zombie : zombies_)
    {
        if (zombie.Alive() && zombie.Type() == ZombieType::Boss)
            return true;
    }
    return false;
}

float Game::BossHealthFraction() const
{
    for (const Zombie &zombie : zombies_)
    {
        if (zombie.Alive() && zombie.Type() == ZombieType::Boss)
        {
            if (zombie.MaxHealth() <= 0)
                return 0.0f;
            return std::clamp(static_cast<float>(zombie.Health()) /
                                  static_cast<float>(zombie.MaxHealth()),
                              0.0f, 1.0f);
        }
    }
    return 0.0f;
}

void Game::ToggleFullscreenMode()
{
    ToggleFullscreen();
    settings_.fullscreen = IsWindowFullscreen();
    if (!settings_.fullscreen)
        SetWindowSize(settings_.windowWidth, settings_.windowHeight);
    SaveSettings();
}

Vector2 Game::ScreenToWorld(Vector2 screenPosition) const
{
    return GetScreenToWorld2D(screenPosition, camera_);
}

Vector2 Game::WorldToScreen(Vector2 worldPosition, const Camera2D &camera)
{
    return {
        (worldPosition.x - camera.target.x) * camera.zoom + camera.offset.x,
        (worldPosition.y - camera.target.y) * camera.zoom + camera.offset.y,
    };
}

void Game::OpenBunker()
{
    bunkerReturnState_ = state_;
    bunkerSelection_ = 0;
    state_ = GameState::Bunker;
}

void Game::OpenSettings(GameState returnState)
{
    settingsReturnState_ = returnState;
    settingsSelection_ = 0;
    settingsControls_ = false;
    awaitingBind_ = -1;
    state_ = GameState::Settings;
}

void Game::UpdateBunker(const InputState &input)
{
    if (input.back)
    {
        state_ = bunkerReturnState_;
        return;
    }

    if (input.navY != 0)
        bunkerSelection_ = (bunkerSelection_ + (input.navY > 0 ? 1 : kBunkerRows - 1)) % kBunkerRows;

    if (input.menuIndex >= 1 && input.menuIndex <= 6)
    {
        BuyMeta(input.menuIndex - 1);
        return;
    }

    if (input.confirm)
    {
        if (bunkerSelection_ == 6)
            state_ = bunkerReturnState_;
        else
            BuyMeta(bunkerSelection_);
    }
}

void Game::BuyMeta(int row)
{
    bool ok = false;

    if (row >= 0 && row < kMetaCount)
    {
        const int cost = MetaUpgradeCost(row, profile_.levels[row]);
        if (cost >= 0 && profile_.scrap >= cost)
        {
            profile_.scrap -= cost;
            profile_.levels[row] += 1;
            ok = true;
        }
    }
    else if (row == 5)
    {
        if (!profile_.smgUnlocked && profile_.scrap >= kSmgUnlockCost)
        {
            profile_.scrap -= kSmgUnlockCost;
            profile_.smgUnlocked = true;
            ok = true;
        }
    }

    if (ok)
    {
        SaveProfile(profilePath_, profile_);
        PlaySoundSafe(assets_.powerUp, assets_.audioReady);
    }
    else
    {
        PlaySoundSafe(assets_.reloadFailed, assets_.audioReady);
    }
}

void Game::UpdateSettings(const InputState &input)
{
    if (awaitingBind_ >= 0)
    {
        if (input.back)
        {
            awaitingBind_ = -1;
            return;
        }

        const int key = input.pressedKey;
        if (key == KEY_ESCAPE)
        {
            awaitingBind_ = -1;
            return;
        }

        if (key > 0 && key != KEY_ESCAPE)
        {
            switch (awaitingBind_)
            {
            case 0: settings_.keys.up = key; break;
            case 1: settings_.keys.down = key; break;
            case 2: settings_.keys.left = key; break;
            case 3: settings_.keys.right = key; break;
            case 4: settings_.keys.reload = key; break;
            case 5: settings_.keys.pause = key; break;
            default: break;
            }
            awaitingBind_ = -1;
            SaveSettings();
            PlaySoundSafe(assets_.pickup, assets_.audioReady);
        }
        return;
    }

    if (settingsControls_)
    {
        if (input.back)
        {
            settingsControls_ = false;
            return;
        }

        if (input.navY != 0)
            controlsSelection_ = (controlsSelection_ + (input.navY > 0 ? 1 : kControlRows - 1)) % kControlRows;

        if (input.confirm)
        {
            if (controlsSelection_ == kControlRows - 1)
                settingsControls_ = false;
            else
                awaitingBind_ = controlsSelection_;
        }
        return;
    }

    if (input.back)
    {
        SaveSettings();
        state_ = settingsReturnState_;
        return;
    }

    if (input.navY != 0)
        settingsSelection_ = (settingsSelection_ + (input.navY > 0 ? 1 : kSettingsRows - 1)) % kSettingsRows;

    bool changed = false;

    if (input.navX != 0)
    {
        const float direction = input.navX > 0 ? 1.0f : -1.0f;
        switch (settingsSelection_)
        {
        case 0:
            settings_.masterVolume = std::clamp(settings_.masterVolume + 0.05f * direction, 0.0f, 1.0f);
            changed = true;
            break;
        case 1:
            settings_.musicVolume = std::clamp(settings_.musicVolume + 0.05f * direction, 0.0f, 1.0f);
            changed = true;
            break;
        case 2:
            settings_.sfxVolume = std::clamp(settings_.sfxVolume + 0.05f * direction, 0.0f, 1.0f);
            changed = true;
            break;
        case 4:
            CycleWindowSize(settings_, input.navX);
            changed = true;
            break;
        case 5:
            settings_.screenShake = direction > 0.0f;
            changed = true;
            break;
        case 6:
            if (GamepadPresent())
            {
                settings_.coop = direction > 0.0f;
                changed = true;
            }
            break;
        default:
            break;
        }
    }

    if (input.confirm)
    {
        switch (settingsSelection_)
        {
        case 3:
            ToggleFullscreenMode();
            changed = true;
            break;
        case 4:
            CycleWindowSize(settings_, 1);
            changed = true;
            break;
        case 5:
            settings_.screenShake = !settings_.screenShake;
            changed = true;
            break;
        case 6:
            if (GamepadPresent())
            {
                settings_.coop = !settings_.coop;
                changed = true;
            }
            break;
        case 7:
            settingsControls_ = true;
            controlsSelection_ = 0;
            break;
        case 8:
            SaveSettings();
            state_ = settingsReturnState_;
            return;
        default:
            break;
        }
    }

    if (changed)
    {
        ApplySettings();
        SaveSettings();
    }
}

void Game::Draw()
{
    if (!initialized_)
        return;

    if (state_ == GameState::Playing)
        HideCursor();
    else
        ShowCursor();

    camera_.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};

    ClearBackground(BLACK);

    switch (state_)
    {
    case GameState::Title:
        DrawTitle();
        break;
    case GameState::LevelingUp:
        DrawLevelingUp();
        break;
    case GameState::GameOver:
        DrawGameOver();
        break;
    case GameState::Playing:
        DrawScene();
        DrawHud();
        break;
    case GameState::Paused:
        DrawScene();
        DrawHud();
        DrawPausedOverlay();
        break;
    case GameState::Bunker:
        DrawBunker();
        break;
    case GameState::Settings:
        DrawSettingsScreen();
        break;
    }
}

void Game::DrawScene()
{
    Camera2D drawCamera = camera_;
    const float scale = static_cast<float>(GetScreenHeight()) / 1080.0f;
    const float shake = (state_ == GameState::Playing && settings_.screenShake)
                            ? shakeTrauma_ * shakeTrauma_ * 16.0f * scale
                            : 0.0f;
    drawCamera.offset.x += std::sin(static_cast<float>(gameTime_) * 137.0f) * shake;
    drawCamera.offset.y += std::cos(static_cast<float>(gameTime_) * 151.0f) * shake;

    BeginMode2D(drawCamera);

    arena_.Draw(assets_.tilesheet);
    DrawBloodDecals();
    arena_.DrawObstacles();
    DrawShockwaves();

    for (const Zombie &zombie : zombies_)
        zombie.Draw();

    for (const Bullet &bullet : bullets_)
        bullet.Draw();

    DrawEnemyShots();

    healthPickup_.Draw();
    ammoPickup_.Draw();

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        const Color tint = slot.player.Alive()
                               ? (i == 0 ? WHITE : Color{150, 220, 255, 255})
                               : Color{120, 50, 50, 255};
        slot.player.Draw(tint);

        if (slot.player.Alive() && slot.muzzleFlash > 0.0f)
        {
            const float alpha = std::min(1.0f, slot.muzzleFlash / 0.07f);
            const float angle = slot.player.Rotation() * DEG2RAD;
            const Vector2 direction{std::cos(angle), std::sin(angle)};
            const Vector2 muzzle{
                slot.player.Center().x + direction.x * 24.0f,
                slot.player.Center().y + direction.y * 24.0f,
            };
            DrawLineEx(slot.player.Center(), muzzle, 3.0f, Fade(YELLOW, alpha));
            DrawCircleV(muzzle, 7.0f + alpha * 3.0f, Fade(ORANGE, 0.85f * alpha));
            DrawCircleV(muzzle, 3.0f + alpha * 1.5f, Fade(YELLOW, alpha));
        }
    }

    DrawDamageNumbers();
    EndMode2D();

    const Rectangle source{0.0f, 0.0f, static_cast<float>(assets_.crosshair.width),
                           static_cast<float>(assets_.crosshair.height)};
    const Vector2 origin{source.width * 0.5f, source.height * 0.5f};

    for (int i = 0; i < playerCount_; ++i)
    {
        PlayerSlot &slot = players_[i];
        if (!slot.player.Alive())
            continue;

        const Rectangle dest{slot.crosshairScreen.x, slot.crosshairScreen.y, source.width, source.height};
        const Color tint = i == 0 ? WHITE : Color{130, 220, 255, 255};
        DrawTexturePro(assets_.crosshair, source, dest, origin, 0.0f, tint);
    }
}

void Game::DrawBloodDecals() const
{
    if (assets_.blood.id == 0)
        return;

    const Rectangle source{0.0f, 0.0f, static_cast<float>(assets_.blood.width),
                           static_cast<float>(assets_.blood.height)};
    for (const BloodDecal &decal : bloodDecals_)
    {
        const float alpha = std::min(1.0f, decal.life / 1.5f) * 0.9f;
        const float size = source.width * decal.scale;
        const Rectangle dest{decal.position.x, decal.position.y, size, size};
        DrawTexturePro(assets_.blood, source, dest, {size * 0.5f, size * 0.5f}, decal.rotation,
                       Fade(WHITE, alpha));
    }
}

void Game::DrawDamageNumbers() const
{
    for (const DamageNumber &number : damageNumbers_)
    {
        const float alpha = std::clamp(number.life / 0.8f, 0.0f, 1.0f);
        const char *text = TextFormat("%d", number.amount);
        const float fontSize = 22.0f;
        const Vector2 size = MeasureTextEx(assets_.font, text, fontSize, 1.0f);
        const Vector2 drawAt{number.position.x - size.x * 0.5f, number.position.y};
        DrawTextEx(assets_.font, text, {drawAt.x + 1.5f, drawAt.y + 1.5f}, fontSize, 1.0f,
                   Fade(BLACK, alpha * 0.8f));
        DrawTextEx(assets_.font, text, drawAt, fontSize, 1.0f, Fade(YELLOW, alpha));
    }
}

void Game::DrawEnemyShots() const
{
    for (const EnemyShot &shot : enemyShots_)
    {
        DrawCircleV(shot.position, 10.0f, Fade(GREEN, 0.22f));
        DrawCircleV(shot.position, 6.0f, Color{110, 255, 120, 255});
        DrawCircleV(shot.position, 3.0f, WHITE);
    }
}

void Game::DrawShockwaves() const
{
    for (const Shockwave &wave : shockwaves_)
    {
        const float progress = 1.0f - std::clamp(wave.life / wave.maxLife, 0.0f, 1.0f);
        const float radius = std::max(2.0f, wave.radius * progress);
        const float alpha = std::clamp(wave.life / wave.maxLife, 0.0f, 1.0f);
        DrawRingLines(wave.position, radius * 0.8f, radius, 0.0f, 360.0f, 48, Fade(wave.color, alpha));
    }
}

void Game::DrawHud()
{
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const float scale = static_cast<float>(height) / 1080.0f;
    const float fontSize = 55.0f * scale;

    const float barAreaHeight = (playerCount_ > 1 ? 68.0f : 34.0f) * scale;
    DrawRectangle(0, 0, width, static_cast<int>(barAreaHeight), Fade(BLACK, 0.45f));

    float barY = 6.0f * scale;
    for (int i = 0; i < playerCount_; ++i)
    {
        DrawWeaponBar(i, barY, i == 0 ? YELLOW : Color{130, 220, 255, 255});
        barY += 32.0f * scale;
    }

    PlayerSlot &first = players_[0];
    DrawTextureEx(assets_.ammoIcon, {20.0f * scale, height * 0.9f}, 0.0f, scale, WHITE);
    DrawTextLine(WeaponName(first.arsenal.Current()), {200.0f * scale, height * 0.845f}, 28.0f * scale,
                 LIGHTGRAY);
    DrawTextLine(TextFormat("%d/%d", first.arsenal.CurrentState().clip, first.arsenal.CurrentState().spare),
                 {200.0f * scale, height * 0.9f}, fontSize, WHITE);

    if (playerCount_ > 1)
    {
        PlayerSlot &second = players_[1];
        DrawTextLine(TextFormat("P2 %s %d/%d", WeaponName(second.arsenal.Current()),
                                second.arsenal.CurrentState().clip, second.arsenal.CurrentState().spare),
                     {width * 0.52f, height * 0.845f}, 26.0f * scale,
                     second.player.Alive() ? Color{130, 220, 255, 255} : GRAY);
    }

    DrawHealthBars();

    DrawTextLine(TextFormat("Score: %d", score_), {20.0f * scale, 7.0f * scale}, fontSize, WHITE);

    const char *hiScoreText = TextFormat("Hi Score: %d", hiScore_);
    const Vector2 hiSize = MeasureTextEx(assets_.font, hiScoreText, fontSize, fontSize * 0.04f);
    DrawTextLine(hiScoreText, {static_cast<float>(width) - hiSize.x - 20.0f * scale, 7.0f * scale},
                 fontSize, WHITE);

    DrawTextLine(TextFormat("Zombies: %d", zombiesAlive_), {width * 0.7f, height * 0.9f}, fontSize, WHITE);
    DrawTextLine(TextFormat("Wave: %d", wave_), {width * 0.85f, height * 0.9f}, fontSize, WHITE);

    DrawBossBar();
    DrawBanner();

    if (damageFlash_ > 0.0f)
    {
        const float alpha = 0.30f * (damageFlash_ / kDamageFlashTime);
        DrawRectangle(0, 0, width, height, Fade(Color{200, 20, 20, 255}, alpha));
    }
}

void Game::DrawWeaponBar(int playerIndex, float y, Color highlight)
{
    const int width = GetScreenWidth();
    const float scale = static_cast<float>(GetScreenHeight()) / 1080.0f;
    const float fontSize = 24.0f * scale;
    const float gap = 24.0f * scale;
    const PlayerSlot &slot = players_[playerIndex];

    std::string labels[kWeaponCount];
    float widths[kWeaponCount];
    float totalWidth = 0.0f;

    for (int i = 0; i < kWeaponCount; ++i)
    {
        const WeaponType type = static_cast<WeaponType>(i);
        if (slot.arsenal.IsUnlocked(type))
        {
            labels[i] = TextFormat("%d %s %d/%d", i + 1, WeaponName(type), slot.arsenal.State(type).clip,
                                   slot.arsenal.State(type).spare);
        }
        else
        {
            labels[i] = TextFormat("%d %s LOCKED", i + 1, WeaponName(type));
        }
        widths[i] = MeasureTextEx(assets_.font, labels[i].c_str(), fontSize, fontSize * 0.04f).x;
        totalWidth += widths[i];
    }
    totalWidth += gap * (kWeaponCount - 1);

    float x = (static_cast<float>(width) - totalWidth) * 0.5f;
    for (int i = 0; i < kWeaponCount; ++i)
    {
        const bool unlocked = slot.arsenal.IsUnlocked(static_cast<WeaponType>(i));
        const bool current = i == static_cast<int>(slot.arsenal.Current()) && slot.player.Alive();
        const Color color = !unlocked ? Fade(GRAY, 0.7f) : (current ? highlight : Fade(LIGHTGRAY, 0.7f));
        DrawTextLine(labels[i].c_str(), {x, y}, fontSize, color);
        x += widths[i] + gap;
    }
}

void Game::DrawHealthBars()
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;
    const float barX = (playerCount_ > 1 ? 0.28f : 0.176f) * width;

    for (int i = 0; i < playerCount_; ++i)
    {
        const PlayerSlot &slot = players_[i];
        const float y = (i == 0) ? height * 0.9f : height * 0.82f;
        const Color color = i == 0 ? RED : Color{190, 90, 255, 255};

        if (i == 1)
            DrawTextLine("P2", {barX - 34.0f * scale, y}, 22.0f * scale,
                         slot.player.Alive() ? Color{130, 220, 255, 255} : GRAY);
        else if (playerCount_ > 1)
            DrawTextLine("P1", {barX - 34.0f * scale, y}, 22.0f * scale, WHITE);

        const float health = std::max(0.0f, slot.player.Health());
        DrawRectangle(static_cast<int>(barX), static_cast<int>(y),
                      static_cast<int>(health * 3.0f * scale), static_cast<int>(50.0f * scale),
                      slot.player.Alive() ? color : Color{80, 80, 80, 255});
    }
}

void Game::DrawBossBar()
{
    if (!HasBoss())
        return;

    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;
    const float barWidth = width * 0.4f;
    const float barHeight = 18.0f * scale;
    const float x = (width - barWidth) * 0.5f;
    const float y = height * 0.085f;

    DrawTextLine("BOSS", {x - 64.0f * scale, y}, 26.0f * scale, Color{255, 120, 120, 255});
    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(barWidth),
                  static_cast<int>(barHeight), Fade(BLACK, 0.6f));
    DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                  static_cast<int>(barWidth * BossHealthFraction()), static_cast<int>(barHeight),
                  Color{220, 40, 40, 255});
    DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(barWidth),
                       static_cast<int>(barHeight), Color{255, 140, 140, 255});
}

void Game::DrawBanner() const
{
    if (bannerTimer_ <= 0.0f || bannerText_.empty())
        return;

    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;
    const float in = std::min(1.0f, (2.4f - bannerTimer_) / 0.25f);
    const float out = std::min(1.0f, bannerTimer_ / 0.6f);
    const float alpha = std::clamp(in, 0.0f, 1.0f) * std::clamp(out, 0.0f, 1.0f);

    DrawTextCentered(bannerText_.c_str(), height * 0.13f, 90.0f * scale, Fade(WHITE, alpha));
    DrawTextCentered("HORDE INCOMING", height * 0.13f + 100.0f * scale, 30.0f * scale,
                     Fade(LIGHTGRAY, alpha));
}

void Game::DrawBackgroundCover()
{
    if (assets_.menuBackground.id == 0)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), DARKGRAY);
        return;
    }

    const Rectangle source{0.0f, 0.0f, static_cast<float>(assets_.menuBackground.width),
                           static_cast<float>(assets_.menuBackground.height)};
    const Rectangle dest{0.0f, 0.0f, static_cast<float>(GetScreenWidth()),
                         static_cast<float>(GetScreenHeight())};
    DrawTexturePro(assets_.menuBackground, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void Game::DrawTitle()
{
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    DrawBackgroundCover();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));

    DrawTextCentered("Zombies", height * 0.18f, 150.0f * scale, WHITE);
    DrawTextCentered("Press Enter to Play", height * 0.48f, 70.0f * scale, WHITE);
    DrawTextCentered(TextFormat("Hi Score: %d      Scrap: %d", hiScore_, profile_.scrap),
                     height * 0.57f, 45.0f * scale, WHITE);
    DrawTextCentered(TextFormat("Runs: %d   Kills: %d   Best wave: %d", profile_.runs, profile_.kills,
                                profile_.bestWave),
                     height * 0.64f, 30.0f * scale, LIGHTGRAY);

    DrawRectangle(0, static_cast<int>(height * 0.68f), GetScreenWidth(),
                  static_cast<int>(height * 0.26f), Fade(BLACK, 0.42f));

    if (GamepadPresent())
    {
        DrawTextCentered(TextFormat("Co-op: %s   (C toggles, player 2 uses a gamepad)",
                                    settings_.coop ? "ON" : "OFF"),
                         height * 0.72f, 28.0f * scale, settings_.coop ? Color{130, 220, 255, 255} : LIGHTGRAY);
    }
    else
    {
        DrawTextCentered("Connect a gamepad to enable co-op", height * 0.72f, 26.0f * scale, LIGHTGRAY);
    }

    DrawTextCentered("M Bunker    S Settings", height * 0.79f, 32.0f * scale, WHITE);
    DrawTextCentered("WASD move   Mouse aim/fire   1-4 weapons   R reload   Esc quit",
                     height * 0.87f, 28.0f * scale, LIGHTGRAY);
}

void Game::DrawLevelingUp()
{
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    DrawBackgroundCover();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.65f));

    if (wave_ == 0)
        DrawTextCentered("GET READY!", height * 0.10f, 70.0f * scale, WHITE);
    else
        DrawTextCentered(TextFormat("WAVE %d CLEARED", wave_), height * 0.10f, 70.0f * scale, WHITE);
    DrawTextCentered("CHOOSE AN UPGRADE", height * 0.19f, 36.0f * scale, LIGHTGRAY);

    DrawLevelUpCards();

    DrawTextCentered("Press 1-3 to choose", height * 0.92f, 26.0f * scale, Fade(LIGHTGRAY, 0.8f));
}

void Game::DrawLevelUpCards()
{
    if (draftSize_ <= 0)
        return;

    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    const float cardWidth = width * 0.62f;
    const float cardX = (width - cardWidth) * 0.5f;
    const float cardHeight = height * 0.15f;
    const float gap = height * 0.025f;
    float y = height * 0.28f;

    for (int i = 0; i < draftSize_; ++i)
    {
        const int id = draft_[i];
        if (id < 0 || id >= kUpgradeCount)
            continue;

        const UpgradeDef &def = UpgradeDefinition(id);
        const Color rarity = RarityColor(def.rarity);
        const Rectangle card{cardX, y, cardWidth, cardHeight};

        DrawRectangleRounded(card, 0.14f, 8, Fade(BLACK, 0.55f));
        DrawRectangleRoundedLinesEx(card, 0.14f, 8, 2.0f * scale, rarity);

        const Rectangle keyBox{card.x + 16.0f * scale,
                               card.y + cardHeight * 0.5f - 22.0f * scale,
                               44.0f * scale, 44.0f * scale};
        DrawRectangleRounded(keyBox, 0.25f, 6, Fade(rarity, 0.22f));
        DrawRectangleRoundedLinesEx(keyBox, 0.25f, 6, 1.0f * scale, rarity);

        const char *keyText = TextFormat("%d", i + 1);
        const float keySize = 28.0f * scale;
        const Vector2 keyMeasure = MeasureTextEx(assets_.font, keyText, keySize, keySize * 0.04f);
        DrawTextEx(assets_.font, keyText,
                   {keyBox.x + keyBox.width * 0.5f - keyMeasure.x * 0.5f,
                    keyBox.y + keyBox.height * 0.5f - keyMeasure.y * 0.5f},
                   keySize, keySize * 0.04f, rarity);

        DrawTextLine(def.name, {card.x + 76.0f * scale, card.y + 16.0f * scale}, 34.0f * scale, rarity);
        DrawTextLine(def.description, {card.x + 76.0f * scale, card.y + cardHeight * 0.58f},
                     24.0f * scale, WHITE);

        if (def.maxStacks < 999)
        {
            const char *stackText = TextFormat("%d/%d", upgradeStacks_[id], def.maxStacks);
            const float stackSize = 22.0f * scale;
            const Vector2 stackMeasure = MeasureTextEx(assets_.font, stackText, stackSize, stackSize * 0.04f);
            DrawTextLine(stackText,
                         {card.x + card.width - stackMeasure.x - 18.0f * scale,
                          card.y + 20.0f * scale},
                         stackSize, Fade(LIGHTGRAY, 0.9f));
        }

        y += cardHeight + gap;
    }
}

void Game::DrawGameOver()
{
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    DrawBackgroundCover();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    DrawTextCentered("GAME OVER", height * 0.22f, 130.0f * scale, RED);
    DrawTextCentered("Press Enter to Play", height * 0.42f, 60.0f * scale, WHITE);
    DrawTextCentered(TextFormat("Score: %d", score_), height * 0.52f, 55.0f * scale, WHITE);
    DrawTextCentered(TextFormat("Hi Score: %d", hiScore_), height * 0.585f, 55.0f * scale, WHITE);
    DrawTextCentered(TextFormat("Reached wave %d", wave_), height * 0.66f, 40.0f * scale, LIGHTGRAY);
    DrawTextCentered(TextFormat("Scrap earned: %d      Total scrap: %d", scrapRun_, profile_.scrap),
                     height * 0.73f, 40.0f * scale, YELLOW);
    DrawTextCentered("M Bunker    S Settings", height * 0.84f, 30.0f * scale, LIGHTGRAY);
}

void Game::DrawPausedOverlay()
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.55f));
    DrawTextCentered("Paused", GetScreenHeight() * 0.36f, GetScreenHeight() * 0.1f, WHITE);
    DrawTextCentered("Press Pause to Resume", GetScreenHeight() * 0.50f, GetScreenHeight() * 0.045f, LIGHTGRAY);
    DrawTextCentered("S Settings", GetScreenHeight() * 0.58f, GetScreenHeight() * 0.035f, LIGHTGRAY);
}

void Game::DrawBunker()
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    DrawBackgroundCover();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.72f));

    DrawTextCentered("BUNKER", height * 0.06f, 72.0f * scale, WHITE);
    DrawTextCentered(TextFormat("SCRAP: %d", profile_.scrap), height * 0.14f, 40.0f * scale, YELLOW);
    DrawTextCentered(TextFormat("Runs: %d   Kills: %d   Best wave: %d", profile_.runs, profile_.kills,
                                profile_.bestWave),
                     height * 0.19f, 26.0f * scale, LIGHTGRAY);

    const float rowX = width * 0.18f;
    const float rowW = width * 0.64f;
    const float rowH = height * 0.075f;
    const float gap = height * 0.008f;
    float y = height * 0.25f;

    for (int i = 0; i < kBunkerRows; ++i)
    {
        const Rectangle row{rowX, y, rowW, rowH};
        const bool selected = i == bunkerSelection_;
        if (selected)
        {
            DrawRectangleRounded(row, 0.12f, 6, Fade(WHITE, 0.10f));
            DrawRectangleRoundedLinesEx(row, 0.12f, 6, 2.0f * scale, YELLOW);
        }
        else
        {
            DrawRectangleRounded(row, 0.12f, 6, Fade(BLACK, 0.35f));
        }

        if (i < kMetaCount)
        {
            const MetaUpgradeDef &def = MetaUpgrade(i);
            const int level = profile_.levels[i];
            const int cost = MetaUpgradeCost(i, level);
            const bool maxed = cost < 0;
            const bool affordable = !maxed && profile_.scrap >= cost;

            DrawTextLine(TextFormat("%d  %s", i + 1, def.name), {row.x + 18.0f * scale, row.y + 8.0f * scale},
                         28.0f * scale, selected ? WHITE : LIGHTGRAY);
            DrawTextLine(def.description, {row.x + 18.0f * scale, row.y + row.height * 0.55f},
                         18.0f * scale, Fade(LIGHTGRAY, 0.8f));

            const char *right = maxed ? "MAXED" : TextFormat("LEVEL %d/%d   %d SCRAP", level, def.maxLevel, cost);
            const float rightSize = 22.0f * scale;
            const Vector2 measure = MeasureTextEx(assets_.font, right, rightSize, rightSize * 0.04f);
            DrawTextLine(right, {row.x + row.width - measure.x - 16.0f * scale, row.y + 12.0f * scale},
                         rightSize, maxed ? GRAY : (affordable ? YELLOW : RED));
        }
        else if (i == 5)
        {
            DrawTextLine("6  SMG BLUEPRINT", {row.x + 18.0f * scale, row.y + 8.0f * scale}, 28.0f * scale,
                         selected ? WHITE : LIGHTGRAY);
            DrawTextLine("Unlocks the high-rate SMG for every run", {row.x + 18.0f * scale, row.y + row.height * 0.55f},
                         18.0f * scale, Fade(LIGHTGRAY, 0.8f));

            const char *right = profile_.smgUnlocked ? "OWNED"
                                                     : TextFormat("%d SCRAP", kSmgUnlockCost);
            const float rightSize = 22.0f * scale;
            const Vector2 measure = MeasureTextEx(assets_.font, right, rightSize, rightSize * 0.04f);
            const bool affordable = !profile_.smgUnlocked && profile_.scrap >= kSmgUnlockCost;
            DrawTextLine(right, {row.x + row.width - measure.x - 16.0f * scale, row.y + 12.0f * scale},
                         rightSize, profile_.smgUnlocked ? GRAY : (affordable ? YELLOW : RED));
        }
        else
        {
            DrawTextLine("7  BACK", {row.x + 18.0f * scale, row.y + 12.0f * scale}, 28.0f * scale,
                         selected ? WHITE : LIGHTGRAY);
        }

        y += rowH + gap;
    }

    DrawTextCentered("1-6 buy   Arrows select   Enter select   B back", height * 0.93f, 24.0f * scale,
                     LIGHTGRAY);
}

void Game::DrawSettingsScreen()
{
    const float height = static_cast<float>(GetScreenHeight());
    const float scale = height / 1080.0f;

    DrawBackgroundCover();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.78f));

    DrawTextCentered("SETTINGS", height * 0.07f, 70.0f * scale, WHITE);

    const float rowX = GetScreenWidth() * 0.2f;
    const float rowW = GetScreenWidth() * 0.6f;
    const float rowH = height * 0.07f;
    const float gap = height * 0.008f;
    float y = height * 0.2f;

    if (settingsControls_)
    {
        const int keys[6] = {settings_.keys.up,    settings_.keys.down, settings_.keys.left,
                             settings_.keys.right, settings_.keys.reload, settings_.keys.pause};

        for (int i = 0; i < kControlRows; ++i)
        {
            const Rectangle row{rowX, y, rowW, rowH};
            const bool selected = i == controlsSelection_;
            if (selected)
            {
                DrawRectangleRounded(row, 0.12f, 6, Fade(WHITE, 0.10f));
                DrawRectangleRoundedLinesEx(row, 0.12f, 6, 2.0f * scale, YELLOW);
            }
            else
            {
                DrawRectangleRounded(row, 0.12f, 6, Fade(BLACK, 0.35f));
            }

            if (i < 6)
            {
                DrawTextLine(kControlNames[i], {row.x + 18.0f * scale, row.y + 12.0f * scale},
                             28.0f * scale, selected ? WHITE : LIGHTGRAY);
                const char *keyText = (awaitingBind_ == i) ? "PRESS A KEY" : KeyName(keys[i]);
                const float valueSize = 26.0f * scale;
                const Vector2 measure = MeasureTextEx(assets_.font, keyText, valueSize, valueSize * 0.04f);
                DrawTextLine(keyText, {row.x + row.width - measure.x - 18.0f * scale, row.y + 12.0f * scale},
                             valueSize, awaitingBind_ == i ? YELLOW : WHITE);
            }
            else
            {
                DrawTextLine("BACK", {row.x + 18.0f * scale, row.y + 12.0f * scale}, 28.0f * scale,
                             selected ? WHITE : LIGHTGRAY);
            }

            y += rowH + gap;
        }

        DrawTextCentered("Arrows select   Enter rebind   B back", height * 0.92f, 24.0f * scale, LIGHTGRAY);
        return;
    }

    const char *labels[kSettingsRows] = {
        "MASTER VOLUME",
        "MUSIC VOLUME",
        "SFX VOLUME",
        "FULLSCREEN",
        "WINDOW SIZE",
        "SCREEN SHAKE",
        "CO-OP (GAMEPAD)",
        "CONTROLS",
        "BACK",
    };

    for (int i = 0; i < kSettingsRows; ++i)
    {
        const Rectangle row{rowX, y, rowW, rowH};
        const bool selected = i == settingsSelection_;
        if (selected)
        {
            DrawRectangleRounded(row, 0.12f, 6, Fade(WHITE, 0.10f));
            DrawRectangleRoundedLinesEx(row, 0.12f, 6, 2.0f * scale, YELLOW);
        }
        else
        {
            DrawRectangleRounded(row, 0.12f, 6, Fade(BLACK, 0.35f));
        }

        DrawTextLine(labels[i], {row.x + 18.0f * scale, row.y + 12.0f * scale}, 28.0f * scale,
                     selected ? WHITE : LIGHTGRAY);

        const char *value = "";
        switch (i)
        {
        case 0: value = TextFormat("%d%%", static_cast<int>(std::lround(settings_.masterVolume * 100.0f))); break;
        case 1: value = TextFormat("%d%%", static_cast<int>(std::lround(settings_.musicVolume * 100.0f))); break;
        case 2: value = TextFormat("%d%%", static_cast<int>(std::lround(settings_.sfxVolume * 100.0f))); break;
        case 3: value = settings_.fullscreen ? "ON" : "OFF"; break;
        case 4: value = TextFormat("%dx%d", settings_.windowWidth, settings_.windowHeight); break;
        case 5: value = settings_.screenShake ? "ON" : "OFF"; break;
        case 6:
            value = GamepadPresent() ? (settings_.coop ? "ON" : "OFF") : "NO GAMEPAD";
            break;
        case 7: value = ">"; break;
        case 8: value = ""; break;
        }

        if (value[0] != '\0')
        {
            const float valueSize = 26.0f * scale;
            const Vector2 measure = MeasureTextEx(assets_.font, value, valueSize, valueSize * 0.04f);
            DrawTextLine(value, {row.x + row.width - measure.x - 18.0f * scale, row.y + 12.0f * scale},
                         valueSize, WHITE);
        }

        y += rowH + gap;
    }

    DrawTextCentered("Arrows adjust   Enter toggle/select   B back", height * 0.92f, 24.0f * scale,
                     LIGHTGRAY);
}

void Game::DrawTextLine(const char *text, Vector2 position, float fontSize, Color color) const
{
    DrawTextEx(assets_.font, text, position, fontSize, fontSize * 0.04f, color);
}

void Game::DrawTextCentered(const char *text, float y, float fontSize, Color color) const
{
    const Vector2 size = MeasureTextEx(assets_.font, text, fontSize, fontSize * 0.04f);
    DrawTextEx(assets_.font, text, {GetScreenWidth() * 0.5f - size.x * 0.5f, y}, fontSize,
               fontSize * 0.04f, color);
}
