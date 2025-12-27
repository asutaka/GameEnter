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
    core/mappers/mapper2.cpp
    core/mappers/mapper3.cpp
    core/mappers/mapper4.cpp
    core/mappers/mapper7.cpp
    core/input/input.cpp
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
target_link_libraries(nes_test PRIVATE nes_core)

# SDL2 Application (Real Emulator)
include(FetchContent)

FetchContent_Declare(
  SDL2
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.zip
)

FetchContent_MakeAvailable(SDL2)

add_executable(nes_app
    desktop/main_sdl.cpp
)

# SDL2 target name might be SDL2::SDL2 or SDL2-static depending on how it's built
if(TARGET SDL2::SDL2)
    target_link_libraries(nes_app PRIVATE nes_core SDL2::SDL2 SDL2::SDL2main)
else()
    target_link_libraries(nes_app PRIVATE nes_core SDL2 SDL2main)
endif()

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

# PPU Debug Tool
add_executable(ppu_debug
    desktop/ppu_debug.cpp
)

target_link_libraries(ppu_debug PRIVATE
    nes_core
)

# PPU Diagnostic Tool (checks register states)
add_executable(ppu_diagnostic
    desktop/ppu_diagnostic.cpp
)

target_link_libraries(ppu_diagnostic PRIVATE
    nes_core
)

# CPU Execution Test (console only, shows debug output)
add_executable(cpu_exec_test
    desktop/cpu_exec_test.cpp
)

target_link_libraries(cpu_exec_test PRIVATE
    nes_core
)

# Force Render Test (manually enables PPUMASK)
add_executable(force_render_test
    desktop/force_render_test.cpp
)

target_link_libraries(force_render_test PRIVATE
    nes_core
)

# Framebuffer Test (checks if any rendering happens)
add_executable(framebuffer_test
    desktop/framebuffer_test.cpp
)

target_link_libraries(framebuffer_test PRIVATE
    nes_core
)

# Palette Test (dumps palette RAM)
add_executable(palette_test
    desktop/palette_test.cpp
)

target_link_libraries(palette_test PRIVATE
    nes_core
)

# Manual Nametable Test (writes test pattern manually)
add_executable(manual_nametable_test
    desktop/manual_nametable_test.cpp
)

target_link_libraries(manual_nametable_test PRIVATE
    nes_core
)

# Nametable Content Check
add_executable(nametable_content_check
    desktop/nametable_content_check.cpp
)

target_link_libraries(nametable_content_check PRIVATE
    nes_core
)

# CHR Tile Check
add_executable(chr_tile_check
    desktop/chr_tile_check.cpp
)

target_link_libraries(chr_tile_check PRIVATE
    nes_core
)

# Force PPUMASK Test
add_executable(force_ppumask_test
    desktop/force_ppumask_test.cpp
)

target_link_libraries(force_ppumask_test PRIVATE
    nes_core
)

# Minimal Render Test
add_executable(minimal_render_test
    desktop/minimal_render_test.cpp
)

target_link_libraries(minimal_render_test PRIVATE
    nes_core
)

message(STATUS "Console test app: ENABLED")
message(STATUS "Game ROM test app: ENABLED")
message(STATUS "PPU test app: ENABLED")
message(STATUS "PPU debug app: ENABLED")
message(STATUS "PPU diagnostic app: ENABLED")
message(STATUS "CPU execution test: ENABLED")
message(STATUS "Force render test: ENABLED")
message(STATUS "Framebuffer test: ENABLED")
message(STATUS "Palette test: ENABLED")

# Desktop Application với SDL2 (Optional)
find_package(SDL2 QUIET)

if(SDL2_FOUND)
    message(STATUS "SDL2 found: ${SDL2_VERSION}")
else()
    message(STATUS "SDL2 not found (optional)")
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
