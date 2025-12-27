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

    std::cout << "=== Minimal Rendering Test ===" << std::endl;
    emu.reset();
    
    // Run a few frames for init
    std::cout << "Running 10 warmup frames..." << std::endl;
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    // Manually setup for rendering
    std::cout << "Setting up palette..." << std::endl;
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x3F);
    emu.memory_.write(0x2006, 0x00);
    emu.memory_.write(0x2007, 0x0F); // BG color = black
    emu.memory_.write(0x2007, 0x30); // Palette 0, color 1 = white
    emu.memory_.write(0x2007, 0x16); // Palette 0, color 2 = red
    emu.memory_.write(0x2007, 0x27); // Palette 0, color 3 = orange
    
    // Write a simple pattern to nametable
    std::cout << "Writing to nametable..." << std::endl;
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x20);
    emu.memory_.write(0x2006, 0x00);
    
    // Checkerboard pattern using tiles 0 and 1
    for (int i = 0; i < 32*30; i++) {
        uint8_t tile = ((i / 32) + (i % 32)) % 2;
        emu.memory_.write(0x2007, tile);
    }
    
    // Set PPUCTRL and PPUMASK
    std::cout << "Enabling rendering..." << std::endl;
    // emu.memory_.write(0x2000, 0x80); // PPUCTRL: NMI enable, BG pattern $0000
    emu.memory_.write(0x2001, 0x1E); // PPUMASK: Show BG + Sprites
    
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
        std::cout << "\n✓ SUCCESS! Rendering works with manual setup!" << std::endl;
        for (const auto& pair : colors) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << pair.first 
                     << ": " << std::dec << pair.second << " pixels" << std::endl;
        }
    } else {
        std::cout << "\n✗ FAILED: Still uniform color" << std::endl;
        std::cout << "Color: #" << std::hex << std::setw(6) << std::setfill('0') 
                 << colors.begin()->first << std::dec << std::endl;
    }
    
    return 0;
}
