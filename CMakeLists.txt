cmake_minimum_required(VERSION 3.15)
project(NESEmulator VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags (cross-platform)
if(MSVC)
    # MSVC flags
    add_compile_options(/W4)  # Warning level 4
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Emulator Core Library
add_library(nes_core STATIC
    core/cpu/cpu.cpp
    core/cpu/opcodes.cpp
    core/ppu/ppu.cpp
    core/apu/apu.cpp
    core/memory/memory.cpp
    core/cartridge/cartridge.cpp
    core/mappers/mapper0.cpp
    core/mappers/mapper1.cpp
    core/mappers/mapper4.cpp
    core/emulator.cpp
)

target_include_directories(nes_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

# Desktop Test Application (Console - No SDL2 needed)
add_executable(nes_test
    desktop/main.cpp
    desktop/disassembler.cpp
)

target_link_libraries(nes_test PRIVATE
    nes_core
)

# Game ROM Test Application
add_executable(game_test
    desktop/game_test.cpp
)

target_link_libraries(game_test PRIVATE
    nes_core
)

# PPU Rendering Test (saves framebuffer as PPM images)
add_executable(ppu_test
    desktop/ppu_test.cpp
)

target_link_libraries(ppu_test PRIVATE
    nes_core
)

message(STATUS "Console test app: ENABLED")
message(STATUS "Game ROM test app: ENABLED")
message(STATUS "PPU test app: ENABLED")

# Desktop Application với SDL2 (Optional)
find_package(SDL2 QUIET)

if(SDL2_FOUND)
    # TODO: Tạo SDL2 app sau khi CPU test pass
    # add_executable(nes_desktop_sdl
    #     desktop/main_sdl.cpp
    #     desktop/renderer_sdl.cpp
    #     desktop/audio_sdl.cpp
    # )
    # 
    # target_link_libraries(nes_desktop_sdl PRIVATE
    #     nes_core
    #     SDL2::SDL2
    # )
    
    message(STATUS "SDL2 found: ${SDL2_VERSION}")
    message(STATUS "SDL2 app: TODO (sau khi CPU test pass)")
else()
    message(STATUS "SDL2 not found (optional)")
    message(STATUS "Console test app vẫn build được!")
endif()

# Tests (Google Test)
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    find_package(GTest QUIET)
    
    if(GTest_FOUND)
        enable_testing()
        
        add_executable(nes_tests
            tests/cpu_tests.cpp
            tests/ppu_tests.cpp
        )
        
        target_link_libraries(nes_tests PRIVATE
            nes_core
            GTest::gtest_main
        )
        
        include(GoogleTest)
        gtest_discover_tests(nes_tests)
        
        message(STATUS "Tests: ENABLED")
    else()
        message(STATUS "Tests: DISABLED (GoogleTest not found)")
    endif()
endif()

# Installation
install(TARGETS nes_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY core/
    DESTINATION include/nes
    FILES_MATCHING PATTERN "*.h"
)
