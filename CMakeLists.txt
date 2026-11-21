cmake_minimum_required(VERSION 3.20)
project(ZombieArenaRaylib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# raylib 6.2 prebuilt for MinGW-w64 is vendored under third_party/raylib.
# Override with -DRAYLIB_ROOT=<path to a raylib install containing include/ and lib/>.
set(RAYLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib" CACHE PATH "raylib root (include/ and lib/)")

add_executable(ZombieArenaRaylib
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Zombie.cpp
    src/Bullet.cpp
    src/Pickup.cpp
    src/Assets.cpp
    src/Arena.cpp
    src/Weapons.cpp
    src/Upgrades.cpp
    src/Waves.cpp
    src/Meta.cpp
    src/Settings.cpp
    src/SmokeTest.cpp
)

target_include_directories(ZombieArenaRaylib PRIVATE src "${RAYLIB_ROOT}/include")

if(WIN32)
    target_link_libraries(ZombieArenaRaylib PRIVATE
        "${RAYLIB_ROOT}/lib/libraylib.a"
        opengl32 gdi32 winmm shell32
    )
else()
    find_package(raylib REQUIRED)
    target_link_libraries(ZombieArenaRaylib PRIVATE raylib)
endif()

if(MINGW)
    target_link_options(ZombieArenaRaylib PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(MSVC)
    target_compile_options(ZombieArenaRaylib PRIVATE /W4)
else()
    target_compile_options(ZombieArenaRaylib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Window-free logic tests for the simulation classes.
add_executable(logic_tests
    tests/logic_tests.cpp
    src/Player.cpp
    src/Zombie.cpp
    src/Bullet.cpp
    src/Pickup.cpp
    src/Arena.cpp
    src/Weapons.cpp
    src/Upgrades.cpp
    src/Waves.cpp
    src/Meta.cpp
    src/Settings.cpp
)
target_include_directories(logic_tests PRIVATE src "${RAYLIB_ROOT}/include")

if(WIN32)
    target_link_libraries(logic_tests PRIVATE
        "${RAYLIB_ROOT}/lib/libraylib.a"
        opengl32 gdi32 winmm shell32
    )
else()
    target_link_libraries(logic_tests PRIVATE raylib)
endif()

if(MINGW)
    target_link_options(logic_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if(MSVC)
    target_compile_options(logic_tests PRIVATE /W4)
else()
    target_compile_options(logic_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Keep assets next to the executable so the game runs from the build tree.
add_custom_command(TARGET ZombieArenaRaylib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:ZombieArenaRaylib>/assets"
    COMMENT "Copying game assets"
)

enable_testing()
add_test(NAME logic COMMAND logic_tests)
add_test(NAME smoke
    COMMAND ZombieArenaRaylib --smoke-test
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:ZombieArenaRaylib>"
)
