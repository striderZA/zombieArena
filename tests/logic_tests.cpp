#include "Arena.h"
#include "Bullet.h"
#include "Meta.h"
#include "Pickup.h"
#include "Player.h"
#include "Rng.h"
#include "Settings.h"
#include "SweptCollision.h"
#include "Upgrades.h"
#include "Waves.h"
#include "Weapons.h"
#include "Zombie.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
int gChecks = 0;
int gFailures = 0;

void Check(bool condition, const char *expression, int line)
{
    ++gChecks;
    if (!condition)
    {
        ++gFailures;
        std::printf("FAIL line %d: %s\n", line, expression);
    }
}

void CheckNear(float actual, float expected, float epsilon, const char *expression, int line)
{
    ++gChecks;
    if (std::fabs(actual - expected) > epsilon)
    {
        ++gFailures;
        std::printf("FAIL line %d: %s (actual %.3f, expected %.3f)\n", line, expression, actual,
                    expected);
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)
#define CHECK_NEAR(actual, expected, epsilon) CheckNear((actual), (expected), (epsilon), #actual, __LINE__)

void TestArsenal()
{
    Arsenal arsenal;
    arsenal.Reset();

    CHECK(arsenal.Current() == WeaponType::Pistol);
    CHECK(arsenal.State(WeaponType::Pistol).clip == 6);
    CHECK(arsenal.State(WeaponType::Pistol).spare == 48);
    CHECK(arsenal.State(WeaponType::Shotgun).clip == 2);
    CHECK(arsenal.State(WeaponType::Shotgun).spare == 12);
    CHECK(arsenal.State(WeaponType::Rifle).clip == 20);
    CHECK(arsenal.State(WeaponType::Rifle).spare == 90);

    CHECK(arsenal.ProjectilesForShot(WeaponType::Pistol) == 1);
    CHECK(arsenal.ProjectilesForShot(WeaponType::Shotgun) == 6);
    CHECK(arsenal.ProjectilesForShot(WeaponType::Rifle) == 1);
    CHECK(arsenal.DamagePerProjectile() == 1);

    arsenal.AddExtraPellet();
    CHECK(arsenal.ProjectilesForShot(WeaponType::Shotgun) == 7);
    CHECK(arsenal.ProjectilesForShot(WeaponType::Rifle) == 1);

    arsenal.AddExtraProjectile();
    CHECK(arsenal.ProjectilesForShot(WeaponType::Pistol) == 2);
    CHECK(arsenal.ProjectilesForShot(WeaponType::Shotgun) == 8);

    arsenal.AddDamage(2);
    CHECK(arsenal.DamagePerProjectile() == 3);

    arsenal.MultiplyFireRate(1.5f);
    CHECK_NEAR(arsenal.EffectiveFireRate(WeaponType::Pistol), 1.5f, 0.001f);
    CHECK_NEAR(arsenal.EffectiveFireRate(WeaponType::Shotgun), 1.2f, 0.001f);

    arsenal.MultiplySpread(0.5f);
    CHECK_NEAR(arsenal.SpreadMultiplier(), 0.5f, 0.001f);

    CHECK(arsenal.EffectiveClip(WeaponType::Pistol) == 6);
    arsenal.ApplyClipUpgrade();
    CHECK(arsenal.EffectiveClip(WeaponType::Pistol) == 9);
    CHECK(arsenal.State(WeaponType::Pistol).clip == 9);
    CHECK(arsenal.EffectiveClip(WeaponType::Shotgun) == 3);
    CHECK(arsenal.State(WeaponType::Shotgun).clip == 3);
    arsenal.ApplyClipUpgrade();
    CHECK(arsenal.EffectiveClip(WeaponType::Pistol) == 12);
    CHECK(arsenal.State(WeaponType::Pistol).clip == 12);

    CHECK(arsenal.AmmoPickupFor(WeaponType::Pistol, 16) == 16);
    CHECK(arsenal.AmmoPickupFor(WeaponType::Shotgun, 16) == 6);
    CHECK(arsenal.AmmoPickupFor(WeaponType::Rifle, 16) == 16);

    arsenal.Cycle(1);
    CHECK(arsenal.Current() == WeaponType::Shotgun);
    arsenal.Cycle(1);
    CHECK(arsenal.Current() == WeaponType::Rifle);
    arsenal.Cycle(-1);
    CHECK(arsenal.Current() == WeaponType::Shotgun);
    arsenal.Switch(WeaponType::Pistol);
    CHECK(arsenal.Current() == WeaponType::Pistol);

    arsenal.Reset();
    CHECK(arsenal.State(WeaponType::Pistol).clip == 6);
    CHECK_NEAR(arsenal.FireRateMultiplier(), 1.0f, 0.001f);
    CHECK(arsenal.ExtraPellets() == 0);
    CHECK(arsenal.ExtraProjectiles() == 0);
    CHECK(arsenal.DamagePerProjectile() == 1);
}

void TestZombie()
{
    const Texture2D none{};

    Zombie chaser;
    chaser.Spawn(ZombieType::Chaser, {100.0f, 100.0f}, none, none, 70.0f, 1);
    CHECK(chaser.Alive());
    CHECK(!chaser.Hit(1)); // health reaches exactly zero, original dies below zero
    CHECK(chaser.Alive());
    CHECK(chaser.Hit(1));
    CHECK(!chaser.Alive());

    Zombie bloater;
    bloater.Spawn(ZombieType::Bloater, {100.0f, 100.0f}, none, none, 40.0f, 5);
    CHECK(!bloater.Hit(5));
    CHECK(bloater.Alive());
    CHECK(bloater.Hit(1));
    CHECK(!bloater.Alive());

    Zombie walker;
    walker.Spawn(ZombieType::Crawler, {100.0f, 100.0f}, none, none, 70.0f, 3);
    walker.Update(0.1f, {200.0f, 100.0f});
    CHECK_NEAR(walker.Position().x, 107.0f, 0.001f);
    CHECK_NEAR(walker.Position().y, 100.0f, 0.001f);

    walker.Knockback({0.0f, 1.0f}, 12.0f);
    CHECK_NEAR(walker.Position().y, 112.0f, 0.001f);

    const Rectangle rect = walker.Rect();
    CHECK_NEAR(rect.width, 50.0f, 0.001f);
    CHECK_NEAR(rect.height, 50.0f, 0.001f);
}

void TestBullet()
{
    Rng rng(42);

    Bullet bullet;
    CHECK(!bullet.InFlight());
    bullet.Shoot({0.0f, 0.0f}, {100.0f, 0.0f}, 0.0f, rng, 3);
    CHECK(bullet.InFlight());
    CHECK(bullet.Damage() == 3);
    CHECK_NEAR(bullet.Direction().x, 1.0f, 0.001f);
    CHECK_NEAR(bullet.Direction().y, 0.0f, 0.001f);
    CHECK_NEAR(bullet.PreviousPosition().x, 0.0f, 0.001f);
    CHECK_NEAR(bullet.PreviousPosition().y, 0.0f, 0.001f);

    bullet.Update(0.5f);
    CHECK_NEAR(bullet.Position().x, 500.0f, 0.01f);
    CHECK_NEAR(bullet.PreviousPosition().x, 0.0f, 0.01f);
    CHECK(bullet.InFlight());
    bullet.Update(0.6f);
    CHECK(!bullet.InFlight());
    CHECK_NEAR(bullet.PreviousPosition().x, 500.0f, 0.01f);

    Bullet spread;
    spread.Shoot({0.0f, 0.0f}, {100.0f, 0.0f}, 90.0f, rng, 1);
    const float angle = std::atan2(spread.Direction().y, spread.Direction().x) * RAD2DEG;
    CHECK(std::fabs(angle) <= 45.1f);

    Bullet degenerate;
    degenerate.Shoot({10.0f, 10.0f}, {10.0f, 10.0f}, 0.0f, rng, 1);
    CHECK_NEAR(degenerate.Direction().x, 1.0f, 0.001f);
}

void TestSweptCollision()
{
    const Rectangle rect{10.0f, 10.0f, 10.0f, 10.0f};
    float t = -1.0f;

    CHECK(SegmentIntersectsRect({0.0f, 15.0f}, {30.0f, 15.0f}, rect, &t));
    CHECK_NEAR(t, 1.0f / 3.0f, 0.001f);
    CHECK(!SegmentIntersectsRect({0.0f, 25.0f}, {30.0f, 25.0f}, rect));

    CHECK(SegmentIntersectsRect({15.0f, 15.0f}, {15.0f, 15.0f}, rect));
    CHECK(!SegmentIntersectsRect({25.0f, 25.0f}, {25.0f, 25.0f}, rect));

    CHECK(SegmentIntersectsRect({15.0f, 0.0f}, {15.0f, 30.0f}, rect, &t));
    CHECK_NEAR(t, 1.0f / 3.0f, 0.001f);

    CHECK(SegmentIntersectsRect({0.0f, 15.0f}, {15.0f, 15.0f}, rect, &t));
    CHECK_NEAR(t, 2.0f / 3.0f, 0.001f);

    CHECK(SegmentIntersectsRect({15.0f, 15.0f}, {30.0f, 15.0f}, rect, &t));
    CHECK_NEAR(t, 0.0f, 0.001f);

    CHECK(!SegmentIntersectsRect({0.0f, 0.0f}, {5.0f, 5.0f}, rect));

    // The 100 px step a 0.1 s frame produces passes completely through the
    // rect, while the endpoint rectangle does not touch it.
    CHECK(SegmentIntersectsRect({0.0f, 15.0f}, {100.0f, 15.0f}, rect, &t));
    CHECK_NEAR(t, 0.1f, 0.001f);
    CHECK(!CheckCollisionRecs(Rectangle{98.0f, 14.0f, 4.0f, 2.0f}, rect));
}

void TestPickup()
{
    Rng rng(7);
    Pickup health(Pickup::Kind::Health);
    CHECK(!health.IsSpawned());
    CHECK(health.Value() == 50);

    health.SetArena({0.0f, 0.0f, 650.0f, 650.0f}, rng);
    CHECK(health.IsSpawned());
    CHECK(health.Position().x >= 50.0f && health.Position().x <= 600.0f);
    CHECK(health.Position().y >= 50.0f && health.Position().y <= 600.0f);

    health.Update(7.0f, rng);
    CHECK(!health.IsSpawned());
    health.Update(7.0f, rng);
    CHECK(health.IsSpawned());

    health.Upgrade();
    CHECK(health.Value() == 75);
    CHECK(health.GotIt() == 75);
    CHECK(!health.IsSpawned());

    Pickup ammo(Pickup::Kind::Ammo);
    CHECK(ammo.Value() == 16);
    ammo.Upgrade();
    CHECK(ammo.Value() == 24);

    ammo.Reset();
    CHECK(ammo.Value() == 16);
    CHECK(!ammo.IsSpawned());
}

void TestPlayer()
{
    Player player;
    player.Spawn({0.0f, 0.0f, 650.0f, 650.0f}, 50.0f);

    CHECK_NEAR(player.Center().x, 325.0f, 0.001f);
    CHECK_NEAR(player.Center().y, 325.0f, 0.001f);
    CHECK_NEAR(player.Health(), 100.0f, 0.001f);

    player.Update(1.0f, {1.0f, 0.0f}, {325.0f, 325.0f});
    CHECK_NEAR(player.Center().x, 525.0f, 0.01f);
    player.Update(1.0f, {1.0f, 0.0f}, {325.0f, 325.0f});
    CHECK_NEAR(player.Center().x, 600.0f, 0.01f); // arena clamp

    CHECK(player.TakeHit(0.0));
    CHECK_NEAR(player.Health(), 90.0f, 0.001f);
    CHECK(!player.TakeHit(0.1));
    CHECK(player.TakeHit(0.3));
    CHECK_NEAR(player.Health(), 80.0f, 0.001f);

    player.UpgradeHealth();
    CHECK_NEAR(player.MaxHealth(), 110.0f, 0.001f);
    CHECK_NEAR(player.Health(), 90.0f, 0.001f);

    const float speedBefore = player.Speed();
    player.UpgradeSpeed();
    CHECK_NEAR(player.Speed(), speedBefore * 1.1f, 0.001f);

    player.ResetStats();
    CHECK_NEAR(player.Health(), 100.0f, 0.001f);
    CHECK_NEAR(player.Speed(), 200.0f, 0.001f);
}

void TestArena()
{
    Arena arena;
    arena.SetWave(1, 1234);
    CHECK_NEAR(arena.Size(), 715.0f, 0.001f);
    CHECK(arena.TileSize() == 50);
    CHECK_NEAR(arena.Bounds().width, 715.0f, 0.001f);

    arena.SetWave(5, 1234);
    CHECK_NEAR(arena.Size(), 975.0f, 0.001f);
}

void TestUpgradeTable()
{
    CHECK(kUpgradeCount == 12);
    CHECK(UpgradeDefinition(kRapidFire).maxStacks == 5);
    CHECK(UpgradeDefinition(kRapidFire).weight > 0);
    CHECK(UpgradeDefinition(kRepairKit).weight == 0);
    CHECK(UpgradeDefinition(kBulletStorm).rarity == UpgradeRarity::Epic);
    CHECK(UpgradeDefinition(kExtendedMag).rarity == UpgradeRarity::Common);
    CHECK(UpgradeDefinition(kFieldMedic).rarity == UpgradeRarity::Rare);
}

void TestRng()
{
    Rng first(123456);
    Rng second(123456);
    CHECK(first.Next() == second.Next());
    CHECK(first.Next() == second.Next());

    Rng other(654321);
    CHECK(other.Next() != Rng(123456).Next());

    for (int i = 0; i < 100; ++i)
    {
        const int value = first.RangeInt(5, 10);
        CHECK(value >= 5 && value < 10);
        const float unit = first.Range(1.0f, 2.0f);
        CHECK(unit >= 1.0f && unit < 2.0f);
    }
}

void TestWaves()
{
    CHECK(WaveZombieCount(1, false) == 5);
    CHECK(WaveZombieCount(2, false) == 7);
    CHECK(WaveZombieCount(1, true) == 8);
    CHECK(WaveZombieCount(5, false) == 11);
    CHECK(WaveZombieCount(5, true) == 18);
    CHECK(WaveZombieCount(10, false) == 5 + 18 - 3 + 1);

    Rng rng(9876);
    for (int wave = 1; wave <= 12; ++wave)
    {
        for (int coop = 0; coop < 2; ++coop)
        {
            const bool isCoop = coop != 0;
            const std::vector<ZombieType> types = ComposeWave(wave, isCoop, rng);
            CHECK(static_cast<int>(types.size()) == WaveZombieCount(wave, isCoop));

            int bosses = 0;
            for (ZombieType type : types)
            {
                if (type == ZombieType::Boss)
                    ++bosses;
                if (wave < 2)
                    CHECK(type != ZombieType::Spitter);
                if (wave < 3)
                    CHECK(type != ZombieType::Sprinter);
                if (wave < 4)
                    CHECK(type != ZombieType::Exploder);
                if (wave < 5)
                    CHECK(type != ZombieType::Boss);
            }
            CHECK(bosses == ((wave % 5 == 0) ? 1 : 0));
        }
    }
}

void TestSpecialZombies()
{
    const Texture2D none{};

    CHECK(Zombie::BaseSpeed(ZombieType::Spitter) == 30.0f);
    CHECK(Zombie::BaseSpeed(ZombieType::Sprinter) == 120.0f);
    CHECK(Zombie::BaseSpeed(ZombieType::Exploder) == 55.0f);
    CHECK(Zombie::BaseSpeed(ZombieType::Boss) == 30.0f);
    CHECK(Zombie::BaseHealth(ZombieType::Boss) == 30);
    CHECK_NEAR(Zombie::ScaleOf(ZombieType::Boss), 2.2f, 0.001f);
    CHECK_NEAR(Zombie::ScaleOf(ZombieType::Sprinter), 0.85f, 0.001f);
    CHECK(Zombie::ScoreValue(ZombieType::Boss) == 100);
    CHECK(Zombie::ScoreValue(ZombieType::Spitter) == 15);
    CHECK(Zombie::ScrapValue(ZombieType::Boss) == 25);
    CHECK(Zombie::ScrapValue(ZombieType::Exploder) == 2);
    CHECK_NEAR(Zombie::ContactDamageValue(ZombieType::Boss), 20.0f, 0.001f);
    CHECK_NEAR(Zombie::ContactDamageValue(ZombieType::Exploder), 15.0f, 0.001f);

    Zombie spitter;
    spitter.Spawn(ZombieType::Spitter, {100.0f, 100.0f}, none, none, 30.0f, 2);
    CHECK(spitter.Ranged());
    CHECK(!spitter.TickAttack(1.0f));
    CHECK(spitter.TickAttack(0.3f)); // 1.2 s cooldown elapsed
    CHECK(!spitter.TickAttack(1.0f));

    Zombie boss;
    boss.Spawn(ZombieType::Boss, {100.0f, 100.0f}, none, none, 30.0f, 30);
    CHECK_NEAR(boss.Scale(), 2.2f, 0.001f);
    CHECK_NEAR(boss.Rect().width, 50.0f * 2.2f, 0.01f);
    CHECK_NEAR(boss.Rect().height, 50.0f * 2.2f, 0.01f);
    CHECK(boss.MaxHealth() == 30);
    CHECK(boss.Health() == 30);
}

void TestArsenalUnlocks()
{
    Arsenal arsenal;
    arsenal.Reset();

    CHECK(kWeaponCount == 4);
    CHECK(arsenal.IsUnlocked(WeaponType::Pistol));
    CHECK(arsenal.IsUnlocked(WeaponType::Shotgun));
    CHECK(arsenal.IsUnlocked(WeaponType::Rifle));
    CHECK(arsenal.IsUnlocked(WeaponType::Smg));
    CHECK(arsenal.State(WeaponType::Smg).clip == 30);
    CHECK(arsenal.State(WeaponType::Smg).spare == 120);
    CHECK(arsenal.ProjectilesForShot(WeaponType::Smg) == 1);
    CHECK_NEAR(arsenal.EffectiveFireRate(WeaponType::Smg), 8.0f, 0.001f);

    arsenal.SetUnlocked(WeaponType::Smg, false);
    CHECK(!arsenal.IsUnlocked(WeaponType::Smg));
    arsenal.Switch(WeaponType::Smg);
    CHECK(arsenal.Current() == WeaponType::Pistol);
    arsenal.Switch(WeaponType::Rifle);
    arsenal.Cycle(1);
    CHECK(arsenal.Current() == WeaponType::Pistol);

    arsenal.SetUnlocked(WeaponType::Smg, true);
    arsenal.Switch(WeaponType::Smg);
    CHECK(arsenal.Current() == WeaponType::Smg);
    arsenal.Cycle(1);
    CHECK(arsenal.Current() == WeaponType::Pistol);
}

void TestMeta()
{
    CHECK(kSmgUnlockCost == 250);
    CHECK(MetaUpgradeCost(kMetaVest, 0) == 40);
    CHECK(MetaUpgradeCost(kMetaVest, 1) == 80);
    CHECK(MetaUpgradeCost(kMetaVest, 4) == 200);
    CHECK(MetaUpgradeCost(kMetaVest, 5) == -1);
    CHECK(MetaUpgrade(kMetaTuning).maxLevel == 2);
    CHECK(MetaUpgrade(kMetaMagnet).maxLevel == 5);
    CHECK(MetaUpgrade(kMetaVest).maxLevel == 5);

    Profile profile;
    CHECK(profile.scrap == 0);
    CHECK(profile.runs == 0);
    CHECK(!profile.smgUnlocked);

    profile.scrap = 777;
    profile.runs = 12;
    profile.kills = 345;
    profile.bestWave = 9;
    profile.totalScrap = 1500;
    profile.smgUnlocked = true;
    profile.levels[kMetaVest] = 3;
    profile.levels[kMetaMagnet] = 2;

    const std::string path = "logic_profile_test.txt";
    std::remove(path.c_str());
    CHECK(SaveProfile(path, profile));

    Profile loaded;
    CHECK(LoadProfile(path, loaded));
    CHECK(loaded.scrap == 777);
    CHECK(loaded.runs == 12);
    CHECK(loaded.kills == 345);
    CHECK(loaded.bestWave == 9);
    CHECK(loaded.totalScrap == 1500);
    CHECK(loaded.smgUnlocked);
    CHECK(loaded.levels[kMetaVest] == 3);
    CHECK(loaded.levels[kMetaMagnet] == 2);
    std::remove(path.c_str());

    Profile missing;
    CHECK(!LoadProfile(path, missing));
}

void TestSettings()
{
    Settings settings;
    CHECK(settings.keys.up == KEY_W);
    CHECK(settings.keys.down == KEY_S);
    CHECK(settings.keys.left == KEY_A);
    CHECK(settings.keys.right == KEY_D);
    CHECK(settings.keys.reload == KEY_R);
    CHECK(settings.keys.pause == KEY_ENTER);
    CHECK(settings.screenShake);

    settings.masterVolume = 0.55f;
    settings.musicVolume = 0.35f;
    settings.sfxVolume = 0.75f;
    settings.fullscreen = false;
    settings.windowWidth = 1920;
    settings.windowHeight = 1080;
    settings.screenShake = false;
    settings.coop = true;
    settings.aimSensitivity = 1.4f;
    settings.keys.up = KEY_UP;
    settings.keys.pause = KEY_P;

    const std::string path = "logic_settings_test.txt";
    std::remove(path.c_str());
    CHECK(SaveSettings(path, settings));

    Settings loaded;
    CHECK(LoadSettings(path, loaded));
    CHECK_NEAR(loaded.masterVolume, 0.55f, 0.001f);
    CHECK_NEAR(loaded.musicVolume, 0.35f, 0.001f);
    CHECK_NEAR(loaded.sfxVolume, 0.75f, 0.001f);
    CHECK(!loaded.fullscreen);
    CHECK(loaded.windowWidth == 1920);
    CHECK(loaded.windowHeight == 1080);
    CHECK(!loaded.screenShake);
    CHECK(loaded.coop);
    CHECK_NEAR(loaded.aimSensitivity, 1.4f, 0.001f);
    CHECK(loaded.keys.up == KEY_UP);
    CHECK(loaded.keys.pause == KEY_P);
    std::remove(path.c_str());

    Settings missing;
    CHECK(!LoadSettings(path, missing));

    CHECK(std::strcmp(KeyName(KEY_A), "A") == 0);
    CHECK(std::strcmp(KeyName(KEY_Z), "Z") == 0);
    CHECK(std::strcmp(KeyName(KEY_R), "R") == 0);
    CHECK(std::strcmp(KeyName(KEY_ENTER), "ENTER") == 0);
    CHECK(std::strcmp(KeyName(KEY_UP), "UP") == 0);
    CHECK(std::strcmp(KeyName(KEY_ZERO), "0") == 0);
    CHECK(std::strcmp(KeyName(KEY_NINE), "9") == 0);
}

void TestPlayerHealth()
{
    Player player;
    player.Spawn({0.0f, 0.0f, 650.0f, 650.0f}, 50.0f);

    CHECK(player.Alive());
    CHECK(player.TakeHit(0.0, 20.0f));
    CHECK_NEAR(player.Health(), 80.0f, 0.001f);
    CHECK(player.Alive());

    CHECK(player.TakeHit(0.3, 90.0f));
    CHECK_NEAR(player.Health(), -10.0f, 0.001f);
    CHECK(!player.Alive());

    player.Revive(0.5f);
    CHECK(player.Alive());
    CHECK_NEAR(player.Health(), 50.0f, 0.001f);

    const float maxBefore = player.MaxHealth();
    player.AddMaxHealth(15.0f, false);
    CHECK_NEAR(player.MaxHealth(), maxBefore + 15.0f, 0.001f);
    CHECK_NEAR(player.Health(), 50.0f, 0.001f);

    const float speedBefore = player.Speed();
    player.MultiplySpeed(1.2f);
    CHECK_NEAR(player.Speed(), speedBefore * 1.2f, 0.001f);
}

} // namespace

int main()
{
    TestArsenal();
    TestZombie();
    TestBullet();
    TestSweptCollision();
    TestPickup();
    TestPlayer();
    TestArena();
    TestUpgradeTable();
    TestRng();
    TestWaves();
    TestSpecialZombies();
    TestArsenalUnlocks();
    TestMeta();
    TestSettings();
    TestPlayerHealth();

    std::printf("logic tests: %d passed, %d failed\n", gChecks - gFailures, gFailures);
    return gFailures == 0 ? 0 : 1;
}
