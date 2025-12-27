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

    std::cout << "=== Full Diagnostic ===" << std::endl;
    emu.reset();
    
    // Warmup frames
    std::cout << "Running 10 warmup frames..." << std::endl;
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    // Run with START button
    std::cout << "Running 300 frames with START..." << std::endl;
    for (int frame = 0; frame < 300; frame++) {
        if (frame >= 180 && frame <= 190) {
            emu.set_controller(0, 0x08);
        } else {
            emu.set_controller(0, 0x00);
        }
        emu.run_frame();
    }
    
    // Check palette
    std::cout << "\n=== Palette Check ===" << std::endl;
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x3F);
    emu.memory_.write(0x2006, 0x00);
    uint8_t dummy = emu.memory_.read(0x2007);
    
    std::cout << "Background palette:" << std::endl;
    for (int i = 0; i < 16; i++) {
        uint8_t color = emu.memory_.read(0x2007);
        std::cout << "  [" << std::dec << std::setw(2) << i << "] = $" 
                 << std::hex << std::setw(2) << std::setfill('0') << (int)color;
        if (i % 4 == 3) std::cout << std::endl;
    }
    
    // Check nametable
    std::cout << "\n=== Nametable Sample ===" << std::endl;
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x20);
    emu.memory_.write(0x2006, 0x00);
    dummy = emu.memory_.read(0x2007);
    
    std::map<uint8_t, int> tiles;
    for (int i = 0; i < 960; i++) {
        uint8_t tile = emu.memory_.read(0x2007);
        tiles[tile]++;
    }
    
    std::cout << "Unique tiles: " << tiles.size() << std::endl;
    if (tiles.size() <= 10) {
        for (const auto& pair : tiles) {
            std::cout << "  Tile $" << std::hex << std::setw(2) << std::setfill('0') 
                     << (int)pair.first << ": " << std::dec << pair.second << " times" << std::endl;
        }
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    std::map<uint32_t, int> colors;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "\n=== Framebuffer ===" << std::endl;
    std::cout << "Unique colors: " << colors.size() << std::endl;
    for (const auto& pair : colors) {
        std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << pair.first 
                 << ": " << std::dec << pair.second << " pixels" << std::endl;
    }
    
    // Now manually fix palette and see if it renders
    std::cout << "\n=== Manually fixing palette ===" << std::endl;
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x3F);
    emu.memory_.write(0x2006, 0x00);
    emu.memory_.write(0x2007, 0x0F); // BG = black
    emu.memory_.write(0x2007, 0x30); // Color 1 = white
    emu.memory_.write(0x2007, 0x16); // Color 2 = red
    emu.memory_.write(0x2007, 0x27); // Color 3 = orange
    
    // Force PPUMASK
    emu.memory_.write(0x2001, 0x1E);
    
    // Run more frames
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    // Check again
    colors.clear();
    fb = emu.get_framebuffer();
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "\n=== After Manual Fix ===" << std::endl;
    std::cout << "Unique colors: " << colors.size() << std::endl;
    
    if (colors.size() > 1) {
        std::cout << "✓ Graphics appeared after manual palette fix!" << std::endl;
        std::cout << "This means game palette is wrong or PPUMASK not enabled." << std::endl;
    } else {
        std::cout << "✗ Still no graphics even after manual fix" << std::endl;
        std::cout << "This means deeper PPU bug." << std::endl;
    }
    
    return 0;
}
