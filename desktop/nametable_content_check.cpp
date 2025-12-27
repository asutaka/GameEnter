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

    std::cout << "=== Nametable Content Check ===" << std::endl;
    emu.reset();
    
    // Run 300 frames with START button
    for (int frame = 0; frame < 300; frame++) {
        if (frame >= 180 && frame <= 190) {
            emu.set_controller(0, 0x08); // START
        } else {
            emu.set_controller(0, 0x00);
        }
        emu.run_frame();
    }
    
    // Check nametable content
    std::cout << "\nReading nametable $2000:" << std::endl;
    emu.memory_.read(0x2002); // Reset latch
    emu.memory_.write(0x2006, 0x20);
    emu.memory_.write(0x2006, 0x00);
    
    uint8_t dummy = emu.memory_.read(0x2007); // Dummy read
    
    int non_zero = 0;
    std::map<uint8_t, int> tile_counts;
    
    for (int i = 0; i < 960; i++) {
        uint8_t tile = emu.memory_.read(0x2007);
        tile_counts[tile]++;
        if (tile != 0x00 && tile != 0x20) non_zero++;
    }
    
    std::cout << "Non-zero/space tiles: " << non_zero << " / 960" << std::endl;
    std::cout << "Unique tiles: " << tile_counts.size() << std::endl;
    
    if (tile_counts.size() <= 10) {
        std::cout << "\nTile distribution:" << std::endl;
        for (const auto& pair : tile_counts) {
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
    
    std::cout << "\nFramebuffer: " << colors.size() << " unique colors" << std::endl;
    
    if (non_zero > 100) {
        std::cout << "\n✓ Nametable HAS content but not rendering!" << std::endl;
        std::cout << "  This means PPUMASK might not be enabled." << std::endl;
        std::cout << "  Or CHR data is empty/wrong." << std::endl;
    } else {
        std::cout << "\n✗ Nametable is mostly empty" << std::endl;
        std::cout << "  Game hasn't written graphics yet." << std::endl;
    }
    
    return 0;
}
