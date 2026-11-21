#include "Assets.h"
#include "Game.h"
#include "Settings.h"
#include "SmokeTest.h"
#include "raylib.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
void PrintUsage()
{
    std::printf(
        "Zombie Arena (raylib)\n"
        "  --windowed [WxH]   start in a window (default 1280x720)\n"
        "  --fullscreen       start fullscreen\n"
        "  --seed N           reproduce a launch (default: random)\n"
        "  --coop             enable co-op (player 2 needs a gamepad)\n"
        "  --no-coop          disable co-op\n"
        "  --wave N           quick-start on wave N (QA/demo)\n"
        "  --profile PATH     profile file (default: assets/gamedata/profile.txt)\n"
        "  --settings PATH    settings file (default: assets/gamedata/settings.txt)\n"
        "  --smoke-test       run the scripted integration test and exit\n"
        "  --screenshots DIR  where --smoke-test writes images (default: smoke)\n"
        "  --help\n");
}

bool ParseWindowSize(const char *value, int &width, int &height)
{
    if (value == nullptr)
        return false;

    int parsedWidth = 0;
    int parsedHeight = 0;
    if (std::sscanf(value, "%dx%d", &parsedWidth, &parsedHeight) != 2)
        return false;
    if (parsedWidth < 320 || parsedHeight < 240)
        return false;

    width = parsedWidth;
    height = parsedHeight;
    return true;
}

float Length(Vector2 v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

int Sign(float value)
{
    return (value > 0.0f) - (value < 0.0f);
}

InputState SampleInput(const Settings &settings)
{
    InputState input;
    const KeyBinds &keys = settings.keys;

    const bool pad = IsGamepadAvailable(0);
    const bool coopPad = pad && settings.coop;

    Vector2 leftStick{0.0f, 0.0f};
    Vector2 rightStick{0.0f, 0.0f};
    bool rtDown = false;
    bool startPressed = false;
    bool aPressed = false;
    bool bPressed = false;
    bool xPressed = false;
    bool yPressed = false;
    bool lbPressed = false;
    bool rbPressed = false;
    int dpadX = 0;
    int dpadY = 0;

    if (pad)
    {
        leftStick = {GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X),
                     GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y)};
        rightStick = {GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X),
                      GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)};
        rtDown = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
                 GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.3f;
        startPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        aPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        bPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        xPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        yPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        lbPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        rbPressed = IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);

        dpadX = (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ? 1 : 0) -
                (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ? 1 : 0);
        dpadY = (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ? 1 : 0) -
                (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) ? 1 : 0);
    }

    PlayerInput &p1 = input.players[0];
    p1.mouseScreen = GetMousePosition();
    p1.move.x = static_cast<float>(IsKeyDown(keys.right) - IsKeyDown(keys.left));
    p1.move.y = static_cast<float>(IsKeyDown(keys.down) - IsKeyDown(keys.up));
    p1.fire = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    p1.reload = IsKeyPressed(keys.reload);

    const int weaponKeys[4] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR};
    for (int i = 0; i < 4; ++i)
    {
        if (IsKeyPressed(weaponKeys[i]))
            p1.weaponSelect = i + 1;
    }
    for (int i = 0; i < 3; ++i)
    {
        if (IsKeyPressed(weaponKeys[i]))
            input.upgrade = i + 1;
    }

    const float wheel = GetMouseWheelMove();
    p1.weaponCycle = (wheel > 0.0f) ? 1 : ((wheel < 0.0f) ? -1 : 0);

    if (pad && !coopPad)
    {
        if (Length(leftStick) > 0.25f)
            p1.move = leftStick;

        const bool mouseMoved = Length(GetMouseDelta()) > 0.5f;
        if (Length(rightStick) > 0.3f && !mouseMoved)
        {
            p1.useStickAim = true;
            p1.stickAim = rightStick;
        }

        p1.fire = p1.fire || rtDown;
        p1.reload = p1.reload || xPressed;
        if (rbPressed)
            p1.weaponCycle = 1;
        else if (lbPressed)
            p1.weaponCycle = -1;
    }

    if (coopPad)
    {
        PlayerInput &p2 = input.players[1];
        p2.mouseScreen = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
        if (Length(leftStick) > 0.25f)
            p2.move = leftStick;
        p2.stickAim = rightStick;
        p2.useStickAim = true;
        p2.fire = rtDown;
        p2.reload = xPressed;
        if (rbPressed)
            p2.weaponCycle = 1;
        else if (lbPressed)
            p2.weaponCycle = -1;
    }

    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) ||
                    aPressed || startPressed;
    input.pausePressed = IsKeyPressed(keys.pause) || startPressed;
    input.back = IsKeyPressed(KEY_BACKSPACE) || bPressed;
    input.toggleCoop = IsKeyPressed(KEY_C) || yPressed;
    input.openBunker = IsKeyPressed(KEY_M) || lbPressed;
    input.openSettings = IsKeyPressed(KEY_S) || rbPressed;

    if (aPressed)
        input.upgrade = 1;
    if (bPressed)
        input.upgrade = 2;
    if (xPressed)
        input.upgrade = 3;

    const int navX = (IsKeyPressed(KEY_RIGHT) ? 1 : 0) - (IsKeyPressed(KEY_LEFT) ? 1 : 0) + dpadX;
    const int navY = (IsKeyPressed(KEY_DOWN) ? 1 : 0) - (IsKeyPressed(KEY_UP) ? 1 : 0) + dpadY;
    input.navX = Sign(static_cast<float>(navX));
    input.navY = Sign(static_cast<float>(navY));

    for (int i = 0; i < 9; ++i)
    {
        if (IsKeyPressed(KEY_ONE + i))
            input.menuIndex = i + 1;
    }

    input.pressedKey = GetKeyPressed();

    return input;
}
} // namespace

int main(int argc, char **argv)
{
    GameOptions options;
    bool smokeTest = false;
    bool seedSpecified = false;
    bool windowModeSpecified = false;
    bool windowSizeSpecified = false;
    bool coopSpecified = false;
    bool coopValue = false;
    int startWave = 0;
    std::string screenshotDir = "smoke";
    std::string profilePath;
    std::string settingsPath;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--windowed") == 0)
        {
            options.fullscreen = false;
            windowModeSpecified = true;
            if (i + 1 < argc && ParseWindowSize(argv[i + 1], options.windowWidth, options.windowHeight))
            {
                windowSizeSpecified = true;
                ++i;
            }
        }
        else if (std::strcmp(argv[i], "--fullscreen") == 0)
        {
            options.fullscreen = true;
            windowModeSpecified = true;
        }
        else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
        {
            options.seed = std::strtoull(argv[++i], nullptr, 0);
            seedSpecified = true;
        }
        else if (std::strcmp(argv[i], "--coop") == 0)
        {
            coopValue = true;
            coopSpecified = true;
        }
        else if (std::strcmp(argv[i], "--no-coop") == 0)
        {
            coopValue = false;
            coopSpecified = true;
        }
        else if (std::strcmp(argv[i], "--wave") == 0 && i + 1 < argc)
        {
            startWave = std::atoi(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--profile") == 0 && i + 1 < argc)
        {
            profilePath = argv[++i];
        }
        else if (std::strcmp(argv[i], "--settings") == 0 && i + 1 < argc)
        {
            settingsPath = argv[++i];
        }
        else if (std::strcmp(argv[i], "--smoke-test") == 0)
        {
            smokeTest = true;
        }
        else if (std::strcmp(argv[i], "--screenshots") == 0 && i + 1 < argc)
        {
            screenshotDir = argv[++i];
        }
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0)
        {
            PrintUsage();
            return 0;
        }
        else
        {
            std::fprintf(stderr, "unknown argument: %s\n", argv[i]);
            PrintUsage();
            return 1;
        }
    }

    const std::string assetRoot = FindAssetRoot();
    if (profilePath.empty())
        profilePath = assetRoot + "/gamedata/profile.txt";
    if (settingsPath.empty())
        settingsPath = assetRoot + "/gamedata/settings.txt";

    if (smokeTest)
    {
        options.fullscreen = false;
        options.windowWidth = 1280;
        options.windowHeight = 720;
        SetTraceLogLevel(LOG_WARNING);

        // Keep tests deterministic and out of the player's real save files.
        profilePath = screenshotDir + "/profile.txt";
        settingsPath = screenshotDir + "/settings.txt";
        std::remove(profilePath.c_str());
        std::remove(settingsPath.c_str());

        options.settings = Settings{};
        options.settings.fullscreen = false;
    }
    else
    {
        LoadSettings(settingsPath, options.settings);
    }

    if (!windowSizeSpecified)
    {
        options.windowWidth = options.settings.windowWidth;
        options.windowHeight = options.settings.windowHeight;
    }
    else
    {
        options.settings.windowWidth = options.windowWidth;
        options.settings.windowHeight = options.windowHeight;
    }
    if (windowModeSpecified)
    {
        options.settings.fullscreen = options.fullscreen;
    }
    else
    {
        options.fullscreen = options.settings.fullscreen;
    }
    if (coopSpecified)
        options.settings.coop = coopValue;

    if (smokeTest)
    {
        options.fullscreen = false;
        options.settings.fullscreen = false;
    }

    if (!seedSpecified)
    {
        if (smokeTest)
            options.seed = 0x5EEDC0DEull;
        else
            options.seed = static_cast<std::uint64_t>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }

    unsigned int flags = 0;
    if (smokeTest)
        flags |= FLAG_WINDOW_HIDDEN;
    else
        flags |= FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE;
    SetConfigFlags(flags);

    InitWindow(options.windowWidth, options.windowHeight, "Zombie Arena");
    if (smokeTest)
        SetTargetFPS(0);
    else
        SetTargetFPS(60);

    SetExitKey(KEY_ESCAPE);
    SetWindowMinSize(800, 450);

    if (options.fullscreen)
    {
        const int monitor = GetCurrentMonitor();
        const int monitorWidth = GetMonitorWidth(monitor);
        const int monitorHeight = GetMonitorHeight(monitor);
        if (monitorWidth > 0 && monitorHeight > 0)
            SetWindowSize(monitorWidth, monitorHeight);
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    options.gamepadAvailable = IsGamepadAvailable(0);
    options.settings.fullscreen = IsWindowFullscreen();
    options.startWave = startWave;
    options.profilePath = profilePath;
    options.settingsPath = settingsPath;

    TraceLog(LOG_INFO, "WINDOW: %dx%d fullscreen=%d gamepad=%d", GetScreenWidth(), GetScreenHeight(),
             IsWindowFullscreen() ? 1 : 0, options.gamepadAvailable ? 1 : 0);
    TraceLog(LOG_INFO, "SEED: %llu", static_cast<unsigned long long>(options.seed));

    Game game;
    if (!game.Init(options))
    {
        std::fprintf(stderr, "failed to load game assets\n");
        game.Shutdown();
        CloseWindow();
        return 1;
    }

    if (smokeTest)
    {
        const int result = RunSmokeTest(game, screenshotDir, options);
        game.Shutdown();
        CloseWindow();
        return result;
    }

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F11))
            game.ToggleFullscreenMode();

        float dt = GetFrameTime();
        if (dt > 0.1f)
            dt = 0.1f;

        const Settings &settings = game.GetSettings();
        const InputState input = SampleInput(settings);
        game.Update(dt, input);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
