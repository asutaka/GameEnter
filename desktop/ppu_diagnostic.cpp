#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Emulator emu;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    std::cout << "=== PPU Diagnostic Tool ===" << std::endl;
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << std::endl;

    emu.reset();

    std::cout << "Running emulator for 3 seconds (180 frames)..." << std::endl;
    std::cout << "Checking if game initializes PPU..." << std::endl;
    std::cout << std::endl;

    // Run for 180 frames (3 seconds) - most games should initialize by then
    for (int frame = 0; frame < 180; frame++) {
        emu.run_frame();

        // Show progress every 30 frames
        if (frame % 30 == 0) {
            std::cout << "Frame " << std::setw(3) << frame << "..." << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Emulation complete. Analyzing framebuffer..." << std::endl;
    std::cout << std::endl;

    // Check if any pixels are rendered
    const uint8_t* framebuffer = emu.get_framebuffer();
    int non_zero_pixels = 0;
    uint32_t first_color = 0;
    
    // Count unique colors
    std::array<uint32_t, 256*240> colors;
    int unique_colors = 0;
    
    for (int i = 0; i < 256 * 240; i++) {
        uint32_t r = framebuffer[i * 4 + 0];
        uint32_t g = framebuffer[i * 4 + 1];
        uint32_t b = framebuffer[i * 4 + 2];
        uint32_t color = (r << 16) | (g << 8) | b;
        
        if (i == 0) {
            first_color = color;
        }
        
        if (color != first_color) {
            non_zero_pixels++;
        }
        
        // Track unique colors
        bool found = false;
        for (int j = 0; j < unique_colors; j++) {
            if (colors[j] == color) {
                found = true;
                break;
            }
        }
        if (!found && unique_colors < 256*240) {
            colors[unique_colors++] = color;
        }
    }

    std::cout << "=== Framebuffer Analysis ===" << std::endl;
    std::cout << "  Background color: #" << std::hex << std::setw(6) << std::setfill('0') 
              << first_color << std::dec << std::endl;
    std::cout << "  Non-background pixels: " << non_zero_pixels << " / " << (256*240) << std::endl;
    std::cout << "  Unique colors: " << unique_colors << std::endl;
    std::cout << std::endl;
    
    

    if (non_zero_pixels > 100) {
        std::cout << "✓ STATUS: RENDERING OK!" << std::endl;
        std::cout << "  Graphics are being rendered successfully." << std::endl;
    } else if (unique_colors == 1) {
        std::cout << "✗ STATUS: NO GRAPHICS - Single color only" << std::endl;
        std::cout << std::endl;
        std::cout << "POSSIBLE CAUSES:" << std::endl;
        std::cout << "  1. Game has NOT enabled PPU rendering yet (PPUMASK)" << std::endl;
        std::cout << "  2. Game is still initializing (needs more time)" << std::endl;
        std::cout << "  3. PPU rendering not working correctly" << std::endl;
        std::cout << std::endl;
        std::cout << "SOLUTIONS:" << std::endl;
        std::cout << "  → Run longer (try 10-15 seconds)" << std::endl;
        std::cout << "  → Press Start button (S key) in SDL app" << std::endl;
        std::cout << "  → Check if game needs controller input to start" << std::endl;
    } else {
        std::cout << "~ STATUS: PARTIAL RENDERING" << std::endl;
        std::cout << "  Some graphics detected but limited variation" << std::endl;
    }

    return 0;
}
