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

    std::cout << "=== PPU Rendering Diagnostic ===" << std::endl;
    emu.reset();
    
    // Run 60 frames (1 second)
    std::cout << "Running 60 frames..." << std::endl;
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    
    // Count unique colors
    std::map<uint32_t, int> color_histogram;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        color_histogram[color]++;
    }
    
    std::cout << "\n=== Color Analysis ===" << std::endl;
    std::cout << "Unique colors found: " << color_histogram.size() << std::endl;
    
    if (color_histogram.size() <= 5) {
        std::cout << "\nColor histogram:" << std::endl;
        for (const auto& pair : color_histogram) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') 
                     << pair.first << ": " << std::dec << pair.second << " pixels" << std::endl;
        }
    }
    
    // Access PPU internals for debugging
    std::cout << "\n=== PPU State ===" << std::endl;
    std::cout << "Scanline: " << emu.ppu_.get_scanline() << std::endl;
    std::cout << "Cycle: " << emu.ppu_.get_cycle() << std::endl;
    
    // Check if rendering is enabled by reading PPU registers
    // We need to add some debug accessors
    
    return 0;
}
