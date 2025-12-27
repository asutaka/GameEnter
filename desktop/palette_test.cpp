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

    std::cout << "=== Palette Check ===" << std::endl;
    emu.reset();
    
    // Run 60 frames
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    // Read palette via PPU registers
    std::cout << "\nReading palette RAM via $2007:" << std::endl;
    
    // Reset latch
    emu.memory_.read(0x2002);
    
    // Set address to $3F00 (palette start)
    emu.memory_.write(0x2006, 0x3F);
    emu.memory_.write(0x2006, 0x00);
    
    // Read palette (first read is dummy)
    emu.memory_.read(0x2007);
    
    std::cout << "Background palettes:" << std::endl;
    for (int i = 0; i < 16; i++) {
        uint8_t color = emu.memory_.read(0x2007);
        std::cout << "  Palette[" << std::dec << std::setw(2) << i << "] = $" 
                 << std::hex << std::setw(2) << std::setfill('0') << (int)color;
        if (i % 4 == 3) std::cout << std::endl;
    }
    
    std::cout << "\nSprite palettes:" << std::endl;
    for (int i = 0; i < 16; i++) {
        uint8_t color = emu.memory_.read(0x2007);
        std::cout << "  Palette[" << std::dec << std::setw(2) << (i+16) << "] = $" 
                 << std::hex << std::setw(2) << std::setfill('0') << (int)color;
        if (i % 4 == 3) std::cout << std::endl;
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    uint32_t first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
    
    std::cout << "\nFramebuffer first pixel: #" << std::hex << std::setw(6) << std::setfill('0') 
             << first_color << std::dec << std::endl;
    
    // Count unique colors
    std::map<uint32_t, int> colors;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "Unique colors in framebuffer: " << colors.size() << std::endl;
    
    return 0;
}
