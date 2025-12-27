#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <map>

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

    std::cout << "=== Force Enable Rendering Test ===" << std::endl;
    emu.reset();
    
    std::cout << "Initial PC: $" << std::hex << std::setw(4) << std::setfill('0') << emu.cpu_.PC << std::endl;
    
    // Run a few frames to let game initialize
    std::cout << "Running 60 frames for init..." << std::endl;
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    std::cout << "Forcing PPUMASK = $1E (BG=1, SPR=1, all enabled)..." << std::endl;
    // Manually write to PPU to force enable rendering
    emu.memory_.write(0x2001, 0x1E);  // Enable BG + Sprites + left 8px
    
    std::cout << "Running 120 more frames with rendering forced on..." << std::endl;
    for (int i = 0; i < 120; i++) {
        if (i % 30 == 0) {
            std::cout << "Frame " << (60 + i) << "..." << std::endl;
        }
        emu.run_frame();
    }
    
    std::cout << "\nChecking framebuffer..." << std::endl;
    const uint8_t* fb = emu.get_framebuffer();
    uint32_t first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
    
    int different = 0;
    std::map<uint32_t, int> color_counts;
    
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        color_counts[color]++;
        if (color != first_color) different++;
    }
    
    std::cout << "Background color: #" << std::hex << std::setw(6) << std::setfill('0') << first_color << std::dec << std::endl;
    std::cout << "Different colored pixels: " << different << " / " << (256*240) << std::endl;
    std::cout << "Unique colors: " << color_counts.size() << std::endl;
    
    if (different > 1000) {
        std::cout << "\n✓✓✓ RENDERING IS WORKING! ✓✓✓" << std::endl;
        std::cout << "Graphics are being rendered successfully!" << std::endl;
        std::cout << "\nTop colors:" << std::endl;
        int count = 0;
        for (auto it = color_counts.rbegin(); it != color_counts.rend() && count < 5; ++it, ++count) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << it->first 
                      << ": " << std::dec << it->second << " pixels" << std::endl;
        }
    } else {
        std::cout << "\n✗ Still no rendering" << std::endl;
        std::cout << "Possible issues:" << std::endl;
        std::cout << "  - PPU rendering logic has bugs" << std::endl;
        std::cout << "  - Pattern tables not loading" << std::endl;
        std::cout << "  - CHR ROM not accessible" << std::endl;
    }
    
    return 0;
}
