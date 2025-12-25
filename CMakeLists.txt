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

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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

# Desktop Application (SDL2)
find_package(SDL2 QUIET)

if(SDL2_FOUND)
    add_executable(nes_desktop
        desktop/main.cpp
        desktop/renderer_sdl.cpp
        desktop/audio_sdl.cpp
    )
    
    target_link_libraries(nes_desktop PRIVATE
        nes_core
        SDL2::SDL2
    )
    
    message(STATUS "Desktop build: ENABLED (SDL2 found)")
else()
    message(STATUS "Desktop build: DISABLED (SDL2 not found)")
    message(STATUS "To enable desktop build, install SDL2:")
    message(STATUS "  Windows: vcpkg install sdl2")
    message(STATUS "  Linux: sudo apt-get install libsdl2-dev")
    message(STATUS "  macOS: brew install sdl2")
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
