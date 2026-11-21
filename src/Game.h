#pragma once

#include "Assets.h"
#include "Arena.h"
#include "Bullet.h"
#include "Meta.h"
#include "Pickup.h"
#include "Player.h"
#include "Rng.h"
#include "Settings.h"
#include "Upgrades.h"
#include "Weapons.h"
#include "Zombie.h"
#include "raylib.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class GameState
{
    Title,
    LevelingUp,
    Playing,
    Paused,
    GameOver,
    Bunker,
    Settings
};

struct PlayerInput
{
    Vector2 mouseScreen{};
    Vector2 move{};
    Vector2 stickAim{};
    bool useStickAim = false;
    bool fire = false;
    bool reload = false;
    int weaponSelect = 0; // 1..4
    int weaponCycle = 0;  // -1 / +1
};

struct InputState
{
    PlayerInput players[2]{};
    bool confirm = false;
    bool pausePressed = false;
    bool back = false;
    bool toggleCoop = false;
    bool openBunker = false;
    bool openSettings = false;
    int upgrade = 0;     // 1..3 draft slot
    int menuIndex = 0;   // 1..9 quick purchase in the bunker
    int navX = 0;        // menu left/right
    int navY = 0;        // menu up/down
    int pressedKey = 0;  // first key pressed this frame (for rebinding)
};

struct GameOptions
{
    bool fullscreen = true;
    int windowWidth = 1280;
    int windowHeight = 720;
    std::uint64_t seed = 0x5EEDC0DEull;
    bool gamepadAvailable = false;
    int startWave = 0; // >0 skips the draft and starts directly on that wave (QA/demo)
    Settings settings{};
    std::string profilePath;
    std::string settingsPath;
};

class Game;
int RunSmokeTest(Game &game, const std::string &screenshotDir, const GameOptions &baseOptions);

class Game
{
public:
    bool Init(const GameOptions &options);
    void Shutdown();

    void Update(float dt, const InputState &input);
    void Draw();

    GameState State() const { return state_; }
    int Wave() const { return wave_; }
    int Score() const { return score_; }
    int HiScore() const { return hiScore_; }
    int ZombiesAlive() const { return zombiesAlive_; }
    int BulletsInFlight() const;
    int PlayerCount() const { return playerCount_; }
    bool CoopEnabled() const { return playerCount_ > 1; }
    int ScrapEarned() const { return scrapRun_; }

    const Settings &GetSettings() const { return settings_; }
    const Profile &GetProfile() const { return profile_; }

    const Assets &GetAssets() const { return assets_; }
    const Player &GetPlayer(int index = 0) const;
    const std::vector<Zombie> &GetZombies() const { return zombies_; }
    const Camera2D &GetCamera() const { return camera_; }

    float PlayerHealth(int index = 0) const;
    int CurrentWeapon(int playerIndex = 0) const;
    int WeaponClip(int weapon) const { return WeaponClipFor(0, weapon); }
    int WeaponSpare(int weapon) const { return WeaponSpareFor(0, weapon); }
    int WeaponClipFor(int playerIndex, int weapon) const;
    int WeaponSpareFor(int playerIndex, int weapon) const;
    int BulletsInClip() const { return WeaponClipFor(0, CurrentWeapon(0)); }
    int BulletsSpare() const { return WeaponSpareFor(0, CurrentWeapon(0)); }
    float FireRateMultiplier() const;

    int DraftSize() const { return draftSize_; }
    int DraftOption(int slot) const;
    int UpgradeStacks(int upgradeId) const;

    bool HasBoss() const;
    float BossHealthFraction() const;
    int EnemyShotsActive() const;
    float ArenaSize() const;
    void ToggleFullscreenMode();

    Vector2 ScreenToWorld(Vector2 screenPosition) const;
    static Vector2 WorldToScreen(Vector2 worldPosition, const Camera2D &camera);

private:
    friend int RunSmokeTest(Game &, const std::string &, const GameOptions &);

    struct PlayerSlot
    {
        Player player;
        Arsenal arsenal;
        double lastShotTime = -1000.0;
        Vector2 aimDirection{1.0f, 0.0f};
        Vector2 crosshairScreen{};
        float muzzleFlash = 0.0f;

        bool Alive() const { return player.Alive(); }
    };

    struct BloodDecal
    {
        Vector2 position{};
        float rotation = 0.0f;
        float scale = 1.0f;
        float life = 0.0f;
    };

    struct DamageNumber
    {
        Vector2 position{};
        float life = 0.0f;
        int amount = 0;
    };

    struct EnemyShot
    {
        Vector2 position{};
        Vector2 velocity{};
        float life = 0.0f;
        int damage = 10;
    };

    struct Shockwave
    {
        Vector2 position{};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 0.0f;
        Color color = WHITE;
    };

    void NewRun();
    void FinishRun();
    void ApplyUpgrade(int slot);
    void GenerateDraft();
    void BeginWave(int wave);
    void UpdatePlaying(float dt, const InputState &input);
    void UpdatePlayer(int index, float dt, const PlayerInput &input);
    void HandleWeaponInput(int index, const PlayerInput &input);
    void HandleReload(int index);
    void FireWeapon(int index, Vector2 target);
    Vector2 AimTarget(int index, const PlayerInput &input);
    void UpdateBullets(float dt);
    void UpdateCollisions();
    void UpdateEnemyShots(float dt);
    void UpdateJuice(float dt);
    void EnterLevelingUp();
    void EnterGameOver();
    void ExplodeAt(Vector2 position, float radius, float damage);
    void SpawnEnemyShot(Vector2 from, Vector2 target);
    void NearestPlayerTarget(Vector2 from, Vector2 &position, float &distance) const;
    void UpdateCamera();

    bool AllPlayersDead() const;
    int AlivePlayerCount() const;
    bool GamepadPresent() const;

    void AddShake(float amount);
    void AddBloodDecal(Vector2 position, float scale, float life);
    void AddDamageNumber(Vector2 position, int amount);
    void AddShockwave(Vector2 position, float radius, float life, Color color);
    void AwardScrap(int baseAmount);

    void ApplyMetaToSlot(PlayerSlot &slot);
    void ApplySettings();
    void SaveSettings();
    void OpenSettings(GameState returnState);
    void UpdateSettings(const InputState &input);
    void OpenBunker();
    void UpdateBunker(const InputState &input);
    void BuyMeta(int row);

    void DrawScene();
    void DrawHud();
    void DrawTitle();
    void DrawLevelingUp();
    void DrawLevelUpCards();
    void DrawGameOver();
    void DrawPausedOverlay();
    void DrawBunker();
    void DrawSettingsScreen();
    void DrawBackgroundCover();
    void DrawBloodDecals() const;
    void DrawDamageNumbers() const;
    void DrawEnemyShots() const;
    void DrawShockwaves() const;
    void DrawHealthBars();
    void DrawWeaponBar(int playerIndex, float y, Color highlight);
    void DrawBossBar();
    void DrawBanner() const;
    void DrawTextLine(const char *text, Vector2 position, float fontSize, Color color) const;
    void DrawTextCentered(const char *text, float y, float fontSize, Color color) const;
    Texture2D ZombieTexture(ZombieType type) const;

    void LoadHiScore();
    void SaveHiScore() const;

    void PlayMusic(Music &music);
    void StopMusic();

    GameOptions options_{};
    Settings settings_;
    Profile profile_;
    std::string profilePath_;
    std::string settingsPath_;

    Assets assets_;
    Arena arena_;
    std::vector<Zombie> zombies_;
    std::array<Bullet, 100> bullets_{};
    Pickup healthPickup_{Pickup::Kind::Health};
    Pickup ammoPickup_{Pickup::Kind::Ammo};
    Camera2D camera_{};

    std::array<PlayerSlot, 2> players_{};
    int playerCount_ = 1;
    int currentBullet_ = 0;


    std::array<int, kUpgradeCount> upgradeStacks_{};
    std::array<int, kDraftSize> draft_{};
    int draftSize_ = 0;

    std::vector<BloodDecal> bloodDecals_;
    std::vector<DamageNumber> damageNumbers_;
    std::vector<EnemyShot> enemyShots_;
    std::vector<Shockwave> shockwaves_;

    GameState state_ = GameState::Title;
    Rng spawnRng_{1};
    Rng pickupRng_{2};
    Rng combatRng_{3};
    std::uint64_t runSeed_ = 0;
    int runCounter_ = 0;
    double gameTime_ = 0.0;

    int score_ = 0;
    int hiScore_ = 0;
    int wave_ = 0;
    int zombiesAlive_ = 0;
    int totalZombies_ = 0;
    int runKills_ = 0;
    int scrapRun_ = 0;
    bool runActive_ = false;
    float scrapMultiplier_ = 1.0f;
    float ammoMetaMultiplier_ = 1.0f;

    float shakeTrauma_ = 0.0f;
    float damageFlash_ = 0.0f;
    float bannerTimer_ = 0.0f;
    std::string bannerText_;

    int settingsSelection_ = 0;
    bool settingsControls_ = false;
    int controlsSelection_ = 0;
    int awaitingBind_ = -1; // 0..5 for the six rebindable actions
    GameState settingsReturnState_ = GameState::Title;
    GameState bunkerReturnState_ = GameState::Title;
    int bunkerSelection_ = 0;

    Music *currentMusic_ = nullptr;
    bool initialized_ = false;
};
