#include "../core/emulator.h"
#include <iostream>
#include <iomanip>

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

   std::cout << "=== CHR ROM Diagnostic Tool ===" << std::endl;
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << std::endl;

    emu.reset();

    // Run a few frames to let game initialize
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }

    std::cout << "Reading CHR ROM data via PPU..." << std::endl;
    std::cout << std::endl;

    // We need to access PPU's internal read function
    // But we don't have access to that from Emulator API!
    // Instead, let's trigger rendering and check if framebuffer changes
    
    std::cout << "Checking if framebuffer has ANY variation..." << std::endl;
    const uint8_t* fb = emu.get_framebuffer();
    
    // Count unique colors
    uint32_t colors[64] = {0};
    int color_count = 0;
    
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) |  (fb[i*4+1] << 8) | fb[i*4+2];
        
        // Check if color already seen
        bool found = false;
        for (int j = 0; j < color_count; j++) {
            if (colors[j] == color) {
                found = true;
                break;
            }
        }
        
        if (!found && color_count < 64) {
            colors[color_count++] = color;
        }
    }
    
    std::cout << "Unique colors found: " << color_count << std::endl;
    std::cout << std::endl;
    
    if (color_count <= 1) {
        std::cout << "❌ PROBLEM: Only 1 color in framebuffer!" << std::endl;
        std::cout << "    This means PPU is NOT rendering anything." << std::endl;
        std::cout << std::endl;
        std::cout << "DIAGNOSTIC:" << std::endl;
        std::cout << "  Background color: #" << std::hex << std::setw(6) << std::setfill('0') << colors[0] << std::dec << std::endl;
        std::cout << std::endl;
        std::cout << "NEXT STEPS:" << std::endl;
        std::cout << "  1. Check if PPU::render_pixel() is being called" << std::endl;
        std::cout << "  2. Check if mask_.show_bg is true" << std::endl;
        std::cout << "  3. Check if CHR ROM has actual data (not all zeros)" << std::endl;
        std::cout << "  4. Check if background tile fetching works" << std::endl;
    } else {
        std::cout << "✅ SUCCESS: Found " << color_count << " colors!" << std::endl;
        std::cout << "    PPU rendering appears to work." << std::endl;
        std::cout << std::endl;
        std::cout << "Colors:" << std::endl;
        for (int i = 0; i < color_count && i < 10; i++) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << colors[i] << std::dec << std::endl;
        }
    }

    return 0;
}
