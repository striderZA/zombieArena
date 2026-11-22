#include "SmokeTest.h"

#include "Game.h"
#include "ObstacleCollision.h"
#include "Waves.h"
#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

namespace
{
struct Checker
{
    int failures = 0;
    int passes = 0;

    void Check(bool condition, const char *message)
    {
        std::printf("[%s] %s\n", condition ? "PASS" : "FAIL", message);
        if (condition)
            ++passes;
        else
            ++failures;
    }
};

constexpr float kDt = 1.0f / 60.0f;

bool ImageHasContent(const std::string &path)
{
    Image image = LoadImage(path.c_str());
    if (image.width <= 0 || image.height <= 0)
    {
        if (image.data != nullptr)
            UnloadImage(image);
        return false;
    }

    int differingSamples = 0;
    const Color first = GetImageColor(image, image.width / 2, image.height / 4);
    for (int i = 0; i < 64; ++i)
    {
        const int x = (i * 37 + 11) % image.width;
        const int y = (i * 53 + 7) % image.height;
        const Color color = GetImageColor(image, x, y);
        if (color.r != first.r || color.g != first.g || color.b != first.b)
            ++differingSamples;
    }

    UnloadImage(image);
    return differingSamples >= 3;
}

void Step(Game &game, const InputState &input, bool draw = true)
{
    game.Update(kDt, input);
    if (draw)
    {
        BeginDrawing();
        game.Draw();
        EndDrawing();
    }
}

void Redraw(Game &game)
{
    for (int i = 0; i < 2; ++i)
    {
        BeginDrawing();
        game.Draw();
        EndDrawing();
    }
}

std::string Screenshot(const std::string &dir, const char *name)
{
    const std::string path = dir + "/" + name;
    TakeScreenshot(path.c_str());
    std::printf("       screenshot: %s\n", path.c_str());
    return path;
}

const Zombie *NearestZombie(const Game &game)
{
    const Zombie *nearest = nullptr;
    float bestDistance = 0.0f;

    const Vector2 player = game.GetPlayer(0).Center();
    for (const Zombie &zombie : game.GetZombies())
    {
        if (!zombie.Alive())
            continue;

        const float dx = zombie.Position().x - player.x;
        const float dy = zombie.Position().y - player.y;
        const float distance = dx * dx + dy * dy;
        if (nearest == nullptr || distance < bestDistance)
        {
            nearest = &zombie;
            bestDistance = distance;
        }
    }

    return nearest;
}

Vector2 AimAt(const Game &game, const Zombie &zombie)
{
    const Vector2 player = game.GetPlayer(0).Center();
    const Vector2 target = zombie.Position();

    const float dx = player.x - target.x;
    const float dy = player.y - target.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    const float flightTime = distance / Bullet::kSpeed;
    const Vector2 predicted{
        target.x + (distance > 0.001f ? dx / distance : 0.0f) * zombie.Speed() * flightTime,
        target.y + (distance > 0.001f ? dy / distance : 0.0f) * zombie.Speed() * flightTime,
    };

    return Game::WorldToScreen(predicted, game.GetCamera());
}

int Sign(float value)
{
    return (value > 0.0f) ? 1 : ((value < 0.0f) ? -1 : 0);
}

InputState PlayerOneInput(Vector2 mouseScreen)
{
    InputState input;
    input.players[0].mouseScreen = mouseScreen;
    return input;
}
} // namespace

int RunSmokeTest(Game &game, const std::string &screenshotDir, const GameOptions &baseOptions)
{
    Checker check;
    MakeDirectory(screenshotDir.c_str());

    const Vector2 center{GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};

    // ---------------------------------------------------------------- single
    InputState input = PlayerOneInput(center);

    Step(game, input);
    Step(game, input);
    Redraw(game);
    const std::string titleShot = Screenshot(screenshotDir, "01-title.png");
    check.Check(game.State() == GameState::Title, "starts on the title screen");
    check.Check(ImageHasContent(titleShot), "title screenshot contains rendered pixels");

    const Assets &assets = game.GetAssets();
    check.Check(assets.player.width == 50 && assets.player.height == 50, "player sprite loaded (50x50)");
    check.Check(assets.tilesheet.width == 50 && assets.tilesheet.height == 200, "tilesheet loaded (50x200)");
    check.Check(assets.font.glyphCount > 0, "custom font loaded");

    InputState confirm;
    confirm.confirm = true;
    Step(game, confirm);
    check.Check(game.State() == GameState::LevelingUp, "Enter starts a new run at the upgrade menu");
    Redraw(game);
    const std::string levelUpShot = Screenshot(screenshotDir, "02-levelup.png");
    check.Check(ImageHasContent(levelUpShot), "level-up screenshot contains rendered pixels");

    const int draft0 = game.DraftOption(0);
    const int draft1 = game.DraftOption(1);
    const int draft2 = game.DraftOption(2);
    const int firstDraftId = draft0;
    check.Check(game.DraftSize() == 3, "three upgrade cards are drafted");
    check.Check(draft0 >= 0 && draft1 >= 0 && draft2 >= 0 && draft0 != draft1 && draft1 != draft2 &&
                    draft0 != draft2,
                "draft offers three distinct cards");
    check.Check(firstDraftId >= 0 && game.UpgradeStacks(firstDraftId) == 0,
                "drafted cards start at zero stacks");

    InputState choose;
    choose.upgrade = 1;
    Step(game, choose);
    check.Check(game.State() == GameState::Playing, "choosing an upgrade starts the wave");
    check.Check(game.Wave() == 1, "wave counter starts at 1");
    check.Check(game.ZombiesAlive() == 5, "wave 1 spawns 5 zombies");
    check.Check(game.ObstaclesActive() >= 2, "wave 1 has arena cover");
    {
        bool centerClear = true;
        for (const Rectangle &rect : game.arena_.Obstacles())
        {
            const Vector2 center{game.ArenaSize() * 0.5f, game.ArenaSize() * 0.5f};
            if (CircleOverlapsRect(center, 150.0f, rect))
                centerClear = false;
        }
        check.Check(centerClear, "cover leaves the player spawn clear");
        check.Check(game.EliteZombiesAlive() == 0, "wave 1 has no elites");
    }
    check.Check(firstDraftId >= 0 && game.UpgradeStacks(firstDraftId) == 1,
                "choosing a card records a stack");
    Redraw(game);
    const std::string waveShot = Screenshot(screenshotDir, "03-wave1.png");
    check.Check(ImageHasContent(waveShot), "wave 1 screenshot contains rendered pixels");

    check.Check(game.CurrentWeapon() == 0, "starts with the pistol");
    check.Check(game.WeaponClip(0) >= 6 && game.WeaponSpare(0) == 48, "pistol starts loaded");
    check.Check(game.WeaponClip(1) >= 2 && game.WeaponSpare(1) == 12, "shotgun has its own ammo pool");
    check.Check(game.WeaponClip(2) >= 20 && game.WeaponSpare(2) == 90, "rifle has its own ammo pool");
    check.Check(game.WeaponClip(3) == 30 && game.WeaponSpare(3) == 120, "SMG pool exists but starts locked");

    InputState swapShotgun = PlayerOneInput(center);
    swapShotgun.players[0].weaponSelect = 2;
    Step(game, swapShotgun);
    check.Check(game.CurrentWeapon() == 1, "key 2 switches to the shotgun");

    const int shotgunClipBefore = game.WeaponClip(1);
    InputState shotgunFire = PlayerOneInput({center.x + 200.0f, center.y});
    shotgunFire.players[0].fire = true;
    Step(game, shotgunFire);
    check.Check(game.BulletsInFlight() >= 6, "shotgun fires a pellet spread");
    check.Check(game.WeaponClip(1) == shotgunClipBefore - 1, "shotgun consumes one shell per shot");
    Redraw(game);
    const std::string shotgunShot = Screenshot(screenshotDir, "04-shotgun.png");
    check.Check(ImageHasContent(shotgunShot), "shotgun screenshot contains rendered pixels");

    InputState idle = PlayerOneInput(center);
    for (int i = 0; i < 67; ++i)
        Step(game, idle);
    check.Check(game.BulletsInFlight() == 0,
                "player bullets travel and expire instead of freezing in mid air");

    InputState swapRifle = PlayerOneInput(center);
    swapRifle.players[0].weaponSelect = 3;
    Step(game, swapRifle);
    check.Check(game.CurrentWeapon() == 2, "key 3 switches to the rifle");

    InputState wheelSwap = PlayerOneInput(center);
    wheelSwap.players[0].weaponCycle = 1;
    Step(game, wheelSwap);
    check.Check(game.CurrentWeapon() == 0, "mouse wheel cycles around the locked SMG");

    InputState swapPistol = PlayerOneInput(center);
    swapPistol.players[0].weaponSelect = 1;
    Step(game, swapPistol);
    check.Check(game.CurrentWeapon() == 0, "key 1 switches back to the pistol");

    InputState fightWeapon = PlayerOneInput(center);
    fightWeapon.players[0].weaponSelect = 3;
    Step(game, fightWeapon);
    check.Check(game.CurrentWeapon() == 2, "rifle is selected for the fight");

    for (int i = 0; i < 60; ++i)
        Step(game, input);
    Redraw(game);
    const std::string gameplayShot = Screenshot(screenshotDir, "05-gameplay.png");
    check.Check(ImageHasContent(gameplayShot), "gameplay screenshot contains rendered pixels");

    InputState pause;
    pause.pausePressed = true;
    Step(game, pause);
    check.Check(game.State() == GameState::Paused, "pause key pauses during play");
    Redraw(game);
    const std::string pausedShot = Screenshot(screenshotDir, "06-paused.png");
    check.Check(ImageHasContent(pausedShot), "paused screenshot contains rendered pixels");

    InputState backInput;
    backInput.back = true;
    InputState pauseSettings;
    pauseSettings.openSettings = true;
    Step(game, pauseSettings);
    check.Check(game.State() == GameState::Settings, "settings opens from the pause screen");
    Step(game, backInput);
    check.Check(game.State() == GameState::Paused, "settings returns to the pause screen");

    Step(game, pause);
    check.Check(game.State() == GameState::Playing, "pause key resumes from pause");

    bool sawBullet = false;
    bool killedSomething = false;
    bool clearedWave = false;
    int startedHealth = static_cast<int>(game.PlayerHealth());
    std::string combatShot;

    for (int step = 0; step < 60 * 90; ++step)
    {
        if (game.State() != GameState::Playing)
            break;

        const Zombie *target = NearestZombie(game);
        InputState bot;
        if (target != nullptr)
        {
            bot.players[0].mouseScreen = AimAt(game, *target);

            const Vector2 player = game.GetPlayer(0).Center();
            const float dx = player.x - target->Position().x;
            const float dy = player.y - target->Position().y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const Vector2 center{game.ArenaSize() * 0.5f, game.ArenaSize() * 0.5f};
            const float cx = center.x - player.x;
            const float cy = center.y - player.y;
            const float centerDistance = std::sqrt(cx * cx + cy * cy);

            Vector2 desired{0.0f, 0.0f};
            if (distance < 320.0f)
            {
                desired = {dx > 0.0f ? 1.0f : (dx < 0.0f ? -1.0f : 0.0f),
                           dy > 0.0f ? 1.0f : (dy < 0.0f ? -1.0f : 0.0f)};
                if (centerDistance > 1.0f)
                {
                    desired.x += cx / centerDistance * 0.35f;
                    desired.y += cy / centerDistance * 0.35f;
                }
            }
            else if (centerDistance > 90.0f)
            {
                desired = {cx / centerDistance * 0.5f, cy / centerDistance * 0.5f};
            }
            bot.players[0].move = desired;
        }
        bot.players[0].fire = true;
        if (game.BulletsInClip() == 0)
            bot.players[0].reload = true;

        game.Update(kDt, bot);

        if (game.BulletsInFlight() > 0)
            sawBullet = true;
        if (game.Score() > 0)
            killedSomething = true;
        if (combatShot.empty() && game.Score() >= 10)
        {
            Redraw(game);
            combatShot = Screenshot(screenshotDir, "07-combat.png");
        }
        if (game.State() == GameState::GameOver)
            break;
    }

    check.Check(sawBullet, "firing creates in-flight bullets");
    check.Check(killedSomething, "at least one zombie was killed by gunfire");
    {
        bool resolved = true;
        for (const Zombie &zombie : game.GetZombies())
        {
            if (zombie.Alive() &&
                CircleBlockedByObstacles(zombie.Position(), zombie.Radius(), game.arena_.Obstacles()))
            {
                resolved = false;
            }
        }
        check.Check(resolved, "zombies resolve out of arena cover");
    }
    check.Check(static_cast<int>(game.PlayerHealth()) <= startedHealth, "player health is tracked");
    check.Check(game.State() != GameState::GameOver, "bot survived the first wave");

    if (combatShot.empty())
    {
        Redraw(game);
        combatShot = Screenshot(screenshotDir, "07-combat.png");
    }
    check.Check(ImageHasContent(combatShot), "combat screenshot contains rendered pixels");

    if (game.State() == GameState::Playing)
    {
        for (int step = 0; step < 60 * 120 && game.State() == GameState::Playing; ++step)
        {
            const Zombie *target = NearestZombie(game);
            InputState bot = PlayerOneInput(center);
            bot.players[0].fire = true;
            if (target != nullptr)
                bot.players[0].mouseScreen = AimAt(game, *target);
            if (game.BulletsInClip() == 0)
                bot.players[0].reload = true;
            game.Update(kDt, bot);
        }
    }

    clearedWave = (game.State() == GameState::LevelingUp && game.Wave() == 1);
    check.Check(clearedWave, "clearing the wave opens the next upgrade menu");

    const int runScore = game.Score();
    check.Check(game.HiScore() >= runScore, "hi score tracks the run score");
    check.Check(runScore >= 50, "score reflects five zombie kills");

    if (clearedWave)
    {
        Redraw(game);
        const std::string clearedShot = Screenshot(screenshotDir, "08-levelup2.png");
        check.Check(ImageHasContent(clearedShot), "post-wave screenshot contains rendered pixels");

        InputState chooseAgain;
        chooseAgain.upgrade = 1;
        Step(game, chooseAgain);
        check.Check(game.State() == GameState::Playing && game.Wave() == 2, "wave 2 starts after the upgrade");
        check.Check(game.ZombiesAlive() == WaveZombieCount(2, false), "wave 2 spawn count matches the wave plan");

        InputState idle = PlayerOneInput(center);
        for (int step = 0; step < 60 * 45 && game.State() == GameState::Playing; ++step)
            game.Update(kDt, idle);

        check.Check(game.State() == GameState::GameOver, "zombies kill a stationary player");
        Redraw(game);
        const std::string gameOverShot = Screenshot(screenshotDir, "09-gameover.png");
        check.Check(ImageHasContent(gameOverShot), "game-over screenshot contains rendered pixels");

        InputState goBunker;
        goBunker.openBunker = true;
        Step(game, goBunker);
        check.Check(game.State() == GameState::Bunker, "bunker opens from game over");
        Step(game, backInput);
        check.Check(game.State() == GameState::GameOver, "bunker returns to game over");

        Step(game, confirm);
        check.Check(game.State() == GameState::LevelingUp && game.Wave() == 0 && game.Score() == 0,
                    "Enter after game over resets the run");
        check.Check(game.CurrentWeapon() == 0 && game.WeaponClip(0) == 6 && game.WeaponSpare(0) == 48,
                    "restart resets the weapon loadout");
        check.Check(firstDraftId >= 0 && game.UpgradeStacks(firstDraftId) == 0,
                    "restart clears upgrade stacks");
        Redraw(game);
        const std::string restartShot = Screenshot(screenshotDir, "10-restart.png");
        check.Check(ImageHasContent(restartShot), "restart screenshot contains rendered pixels");
    }

    check.Check(game.GetProfile().scrap >= 12, "kills and wave clears award scrap");

    game.Shutdown();

    // ---------------------------------------------------- swept bullet hit
    GameOptions sweepOptions = baseOptions;
    sweepOptions.settings.coop = false;
    sweepOptions.gamepadAvailable = false;
    sweepOptions.startWave = 1;
    sweepOptions.profilePath = screenshotDir + "/sweep_profile.txt";
    sweepOptions.settingsPath = screenshotDir + "/sweep_settings.txt";
    std::remove(sweepOptions.profilePath.c_str());
    std::remove(sweepOptions.settingsPath.c_str());

    Game sweepGame;
    const bool sweepReady = sweepGame.Init(sweepOptions);
    check.Check(sweepReady && sweepGame.State() == GameState::Playing,
                "swept-collision game starts on wave 1");

    if (sweepReady && sweepGame.State() == GameState::Playing)
    {
        const Vector2 player = sweepGame.GetPlayer(0).Center();
        const Vector2 targetPosition{player.x + 60.0f, player.y};

        Zombie target;
        target.Spawn(ZombieType::Bloater, targetPosition,
                     sweepGame.ZombieTexture(ZombieType::Bloater), sweepGame.assets_.blood,
                     Zombie::BaseSpeed(ZombieType::Bloater), Zombie::BaseHealth(ZombieType::Bloater));

        sweepGame.zombies_.clear();
        sweepGame.zombies_.push_back(target);
        sweepGame.zombiesAlive_ = 1;
        sweepGame.totalZombies_ = 1;

        const int healthBefore = sweepGame.zombies_[0].Health();
        InputState sweepFire =
            PlayerOneInput(Game::WorldToScreen(targetPosition, sweepGame.camera_));
        sweepFire.players[0].fire = true;

        // One frame at the main loop's dt clamp: the bullet moves 100 px from
        // the muzzle to well past the zombie without its endpoint touching it.
        sweepGame.Update(0.1f, sweepFire);

        Redraw(sweepGame);
        const std::string sweepShot = Screenshot(screenshotDir, "15-bullet-sweep.png");
        check.Check(ImageHasContent(sweepShot), "swept-collision screenshot contains pixels");
        check.Check(sweepGame.zombies_[0].Health() < healthBefore,
                    "a 100 px bullet step hits a zombie swept between endpoints");
    }

    sweepGame.Shutdown();

    // ---------------------------------------------------------------- co-op
    Profile bunkerProfile;
    bunkerProfile.scrap = 500;
    bunkerProfile.runs = 3;
    bunkerProfile.kills = 20;
    bunkerProfile.bestWave = 4;
    bunkerProfile.totalScrap = 500;
    check.Check(SaveProfile(baseOptions.profilePath, bunkerProfile), "bunker test profile saved");

    GameOptions coopOptions = baseOptions;
    coopOptions.settings.coop = true;
    coopOptions.gamepadAvailable = true;
    coopOptions.startWave = 0;

    Game coopGame;
    check.Check(coopGame.Init(coopOptions), "co-op game initializes");

    InputState menu = PlayerOneInput(center);
    Step(coopGame, menu);

    InputState openSettings;
    openSettings.openSettings = true;
    Step(coopGame, openSettings);
    check.Check(coopGame.State() == GameState::Settings, "settings screen opens from the title");
    Redraw(coopGame);
    const std::string settingsShot = Screenshot(screenshotDir, "11-settings.png");
    check.Check(ImageHasContent(settingsShot), "settings screenshot contains rendered pixels");

    for (int i = 0; i < 7; ++i)
    {
        InputState nav;
        nav.navY = 1;
        Step(coopGame, nav);
    }
    InputState enterControls;
    enterControls.confirm = true;
    Step(coopGame, enterControls);
    Redraw(coopGame);
    const std::string controlsShot = Screenshot(screenshotDir, "11b-controls.png");
    check.Check(ImageHasContent(controlsShot), "controls screenshot contains rendered pixels");

    InputState bindStart;
    bindStart.confirm = true;
    Step(coopGame, bindStart);
    InputState bindUp;
    bindUp.pressedKey = KEY_UP;
    Step(coopGame, bindUp);
    check.Check(coopGame.GetSettings().keys.up == KEY_UP, "keybinding captures a new key");
    Step(coopGame, bindStart);
    InputState bindW;
    bindW.pressedKey = KEY_W;
    Step(coopGame, bindW);
    check.Check(coopGame.GetSettings().keys.up == KEY_W, "keybinding can be restored");

    InputState back;
    back.back = true;
    Step(coopGame, back);
    check.Check(coopGame.State() == GameState::Settings, "back leaves the controls submenu");
    Step(coopGame, back);
    check.Check(coopGame.State() == GameState::Title, "back returns from settings");

    InputState openBunker;
    openBunker.openBunker = true;
    Step(coopGame, openBunker);
    check.Check(coopGame.State() == GameState::Bunker, "bunker screen opens from the title");
    Redraw(coopGame);
    const std::string bunkerShot = Screenshot(screenshotDir, "12-bunker.png");
    check.Check(ImageHasContent(bunkerShot), "bunker screenshot contains rendered pixels");

    InputState buy;
    buy.confirm = true;
    Step(coopGame, buy);
    check.Check(coopGame.GetProfile().scrap == 460 && coopGame.GetProfile().levels[kMetaVest] == 1,
                "bunker purchase spends scrap and adds a meta level");

    for (int i = 0; i < 5; ++i)
    {
        InputState nav;
        nav.navY = 1;
        Step(coopGame, nav);
    }
    Step(coopGame, buy);
    check.Check(coopGame.GetProfile().smgUnlocked && coopGame.GetProfile().scrap == 210,
                "bunker purchase unlocks the SMG");

    Step(coopGame, back);
    check.Check(coopGame.State() == GameState::Title, "back returns from the bunker");

    InputState coopConfirm;
    coopConfirm.confirm = true;
    Step(coopGame, coopConfirm);
    check.Check(coopGame.State() == GameState::LevelingUp, "co-op run reaches the draft");

    InputState coopChoose;
    coopChoose.upgrade = 1;
    Step(coopGame, coopChoose);
    check.Check(coopGame.State() == GameState::Playing, "co-op wave starts");
    check.Check(coopGame.PlayerCount() == 2 && coopGame.CoopEnabled(), "two players are active");
    check.Check(coopGame.PlayerHealth(0) > 0.0f && coopGame.PlayerHealth(1) > 0.0f, "both players are alive");
    check.Check(coopGame.PlayerHealth(0) > 100.0f, "meta health applies to a co-op player");
    check.Check(coopGame.ZombiesAlive() == WaveZombieCount(1, true), "co-op wave 1 uses the scaled horde");

    InputState smgSelect;
    smgSelect.players[0].weaponSelect = 4;
    Step(coopGame, smgSelect);
    check.Check(coopGame.CurrentWeapon(0) == 3, "purchased SMG can be equipped");

    const Vector2 p2Start = coopGame.GetPlayer(1).Center();
    InputState p2Move;
    p2Move.players[1].move = {0.0f, 1.0f};
    for (int i = 0; i < 100; ++i)
        Step(coopGame, p2Move);
    check.Check(coopGame.GetPlayer(1).Center().y > p2Start.y + 5.0f, "player 2 moves with the gamepad stick");
    check.Check(coopGame.GetCamera().zoom < 0.99f, "co-op camera zooms out for separated players");

    const int p2ClipBefore = coopGame.WeaponClipFor(1, 0);
    InputState p2Fire;
    p2Fire.players[1].useStickAim = true;
    p2Fire.players[1].stickAim = {1.0f, 0.0f};
    p2Fire.players[1].fire = true;
    Step(coopGame, p2Fire);
    check.Check(coopGame.WeaponClipFor(1, 0) == p2ClipBefore - 1, "player 2 fires from the gamepad");
    Redraw(coopGame);
    const std::string coopShot = Screenshot(screenshotDir, "13-coop.png");
    check.Check(ImageHasContent(coopShot), "co-op screenshot contains rendered pixels");

    InputState idleCoop;
    for (int step = 0; step < 60 * 30 && coopGame.State() == GameState::Playing; ++step)
        coopGame.Update(kDt, idleCoop);
    check.Check(coopGame.State() == GameState::GameOver, "co-op ends when both players die");

    coopGame.Shutdown();

    // ------------------------------------- last-kill exploder death precedence
    GameOptions deathOptions = baseOptions;
    deathOptions.settings.coop = false;
    deathOptions.gamepadAvailable = false;
    deathOptions.startWave = 1;
    deathOptions.profilePath = screenshotDir + "/death_profile.txt";
    deathOptions.settingsPath = screenshotDir + "/death_settings.txt";
    std::remove(deathOptions.profilePath.c_str());
    std::remove(deathOptions.settingsPath.c_str());

    Game deathGame;
    const bool deathReady = deathGame.Init(deathOptions);
    check.Check(deathReady && deathGame.State() == GameState::Playing,
                "last-kill death game starts on wave 1");

    if (deathReady && deathGame.State() == GameState::Playing)
    {
        const Vector2 player = deathGame.GetPlayer(0).Center();
        const Vector2 exploderPosition{player.x + 90.0f, player.y};

        deathGame.zombies_.clear();
        Zombie exploder;
        exploder.Spawn(ZombieType::Exploder, exploderPosition,
                       deathGame.ZombieTexture(ZombieType::Exploder), deathGame.assets_.blood,
                       Zombie::BaseSpeed(ZombieType::Exploder), 0);
        deathGame.zombies_.push_back(exploder);
        deathGame.zombiesAlive_ = 1;
        deathGame.totalZombies_ = 1;

        deathGame.players_[0].player.TakeHit(deathGame.gameTime_ - 1.0, 90.0f);
        check.Check(static_cast<int>(deathGame.PlayerHealth()) == 10,
                    "player is at 10 health for the blast test");

        InputState idleDeath;
        for (int step = 0; step < 30 && deathGame.State() == GameState::Playing; ++step)
        {
            InputState shot = PlayerOneInput(Game::WorldToScreen(exploderPosition, deathGame.camera_));
            shot.players[0].fire = true;
            deathGame.Update(kDt, shot);
        }

        check.Check(deathGame.State() == GameState::GameOver,
                    "exploder blast on the last kill ends the run");
        check.Check(deathGame.ZombiesAlive() == 0, "the last zombie is still counted dead");

        deathGame.Update(kDt, idleDeath);
        check.Check(deathGame.State() == GameState::GameOver,
                    "the wave-clear draft does not override game over");
    }

    deathGame.Shutdown();

    // ---------------------------------------------------------------- boss
    GameOptions bossOptions = baseOptions;
    bossOptions.settings.coop = false;
    bossOptions.gamepadAvailable = false;
    bossOptions.startWave = 5;

    Game bossGame;
    check.Check(bossGame.Init(bossOptions), "boss quick-start initializes");
    check.Check(bossGame.GetProfile().levels[kMetaVest] == 1 && bossGame.GetProfile().smgUnlocked,
                "profile and purchases persist across game instances");
    check.Check(bossGame.PlayerHealth(0) > 100.0f, "meta health applies on a new game instance");
    check.Check(bossGame.State() == GameState::Playing && bossGame.Wave() == 5, "quick-start opens wave 5");
    check.Check(bossGame.ZombiesAlive() == WaveZombieCount(5, false), "wave 5 count matches the wave plan");
    check.Check(bossGame.HasBoss(), "wave 5 spawns a boss");
    check.Check(bossGame.ObstaclesActive() >= 2, "wave 5 has arena cover");
    check.Check(bossGame.EliteZombiesAlive() >= 1, "wave 5 includes an elite zombie");
    int spitters = 0;
    for (const Zombie &zombie : bossGame.GetZombies())
    {
        if (zombie.Alive() && zombie.Type() == ZombieType::Spitter)
            ++spitters;
    }
    check.Check(spitters > 0, "wave 5 includes spitters");

    InputState idleBoss;
    for (int step = 0; step < 120 && bossGame.State() == GameState::Playing; ++step)
        bossGame.Update(kDt, idleBoss);
    check.Check(bossGame.EnemyShotsActive() > 0, "spitters fire projectiles");
    check.Check(bossGame.BossHealthFraction() > 0.99f, "boss starts at full health");

    const Zombie *boss = nullptr;
    for (const Zombie &zombie : bossGame.GetZombies())
    {
        if (zombie.Alive() && zombie.Type() == ZombieType::Boss)
            boss = &zombie;
    }
    check.Check(boss != nullptr, "boss zombie is present");

    if (boss != nullptr)
    {
        const Vector2 player = bossGame.GetPlayer(0).Center();
        const Vector2 bossPosition = boss->Position();
        InputState approach;
        approach.players[0].move = {static_cast<float>(Sign(bossPosition.x - player.x)),
                                    static_cast<float>(Sign(bossPosition.y - player.y))};
        for (int step = 0; step < 60 && bossGame.State() == GameState::Playing; ++step)
            bossGame.Update(kDt, approach);

        Redraw(bossGame);
        const std::string bossShot = Screenshot(screenshotDir, "14-boss.png");
        check.Check(ImageHasContent(bossShot), "boss screenshot contains rendered pixels");

        const Vector2 bossScreen = Game::WorldToScreen(boss->Position(), bossGame.GetCamera());
        check.Check(bossScreen.x >= 0.0f && bossScreen.x <= GetScreenWidth() && bossScreen.y >= 0.0f &&
                        bossScreen.y <= GetScreenHeight(),
                    "boss is inside the viewport");
    }

    bossGame.Shutdown();

    const std::string summaryPath = screenshotDir + "/summary.txt";
    std::ofstream summary(summaryPath);
    if (summary.is_open())
    {
        summary << "passes=" << check.passes << "\n";
        summary << "failures=" << check.failures << "\n";
        summary << "runScore=" << runScore << "\n";
        summary << "profileScrap=" << bossGame.GetProfile().scrap << "\n";
        summary << "screenshots=" << screenshotDir << "\n";
    }

    std::printf("\nsmoke test: %d passed, %d failed\n", check.passes, check.failures);
    return check.failures == 0 ? 0 : 1;
}
