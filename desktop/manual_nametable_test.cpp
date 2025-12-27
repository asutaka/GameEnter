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

    std::cout << "=== Manual Nametable Write Test ===" << std::endl;
    emu.reset();
    
    // Run a few frames for init
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    std::cout << "Manually writing test pattern to nametable..." << std::endl;
    
    // Reset PPU address latch
    emu.memory_.read(0x2002);
    
    // Write to palette first
    std::cout << "Writing palette..." << std::endl;
    emu.memory_.write(0x2006, 0x3F); // PPUADDR high
    emu.memory_.write(0x2006, 0x00); // PPUADDR low = $3F00
    
    // Write some palette colors
    emu.memory_.write(0x2007, 0x0F); // Background color = black
    emu.memory_.write(0x2007, 0x30); // Palette 0, color 1 = white
    emu.memory_.write(0x2007, 0x16); // Palette 0, color 2 = red
    emu.memory_.write(0x2007, 0x27); // Palette 0, color 3 = orange
    
    // Reset latch
    emu.memory_.read(0x2002);
    
    // Write to nametable
    std::cout << "Writing nametable pattern..." << std::endl;
    emu.memory_.write(0x2006, 0x20); // PPUADDR high
    emu.memory_.write(0x2006, 0x00); // PPUADDR low = $2000
    
    // Write a simple pattern (tile indices)
    for (int i = 0; i < 32*30; i++) {
        // Checkerboard pattern using tiles 0 and 1
        uint8_t tile = ((i / 32) + (i % 32)) % 2;
        emu.memory_.write(0x2007, tile);
    }
    
    // Force enable rendering
    std::cout << "Forcing PPUMASK = $1E..." << std::endl;
    emu.memory_.write(0x2001, 0x1E);
    
    // Run frames
    std::cout << "Running 60 frames..." << std::endl;
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    std::map<uint32_t, int> colors;
    
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Unique colors: " << colors.size() << std::endl;
    
    if (colors.size() > 1) {
        std::cout << "\n✓✓✓ SUCCESS! Manual write works! ✓✓✓" << std::endl;
        std::cout << "\nColors found:" << std::endl;
        for (const auto& pair : colors) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << pair.first 
                     << ": " << std::dec << pair.second << " pixels" << std::endl;
        }
    } else {
        std::cout << "\n✗ FAILED: Still uniform color" << std::endl;
        std::cout << "Color: #" << std::hex << std::setw(6) << std::setfill('0') 
                 << colors.begin()->first << std::dec << std::endl;
        std::cout << "\nThis means PPU rendering logic has a fundamental bug." << std::endl;
    }
    
    return 0;
}
