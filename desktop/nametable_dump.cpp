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

    std::cout << "=== Nametable Dump Tool ===" << std::endl;
    emu.reset();

    // Run 60 frames to let game write to nametable
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    std::cout << "Dumping Nametable 0 ($2000-$23BF):" << std::endl;
    
    // We can't access PPU memory directly easily without hacking core
    // But we can use the PPU read register $2007 via CPU!
    
    // 1. Read PPUSTATUS to reset latch
    emu.memory_.read(0x2002);
    
    // 2. Set PPUADDR to $2000
    emu.memory_.write(0x2006, 0x20);
    emu.memory_.write(0x2006, 0x00);
    
    // 3. Read PPUDATA
    // First read is dummy buffer
    emu.memory_.read(0x2007);
    
    int non_space = 0;
    
    for (int i = 0; i < 960; i++) { // 30 rows * 32 cols
        uint8_t tile = emu.memory_.read(0x2007);
        
        if (i % 32 == 0) {
            std::cout << std::endl << "Row " << std::setw(2) << (i/32) << ": ";
        }
        
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)tile << " ";
        
        if (tile != 0x20 && tile != 0x00) non_space++;
    }
    
    std::cout << std::dec << std::endl << std::endl;
    std::cout << "Non-space tiles: " << non_space << std::endl;
    
    if (non_space == 0) {
        std::cout << "❌ NAMETABLE IS EMPTY (All 00 or 20)" << std::endl;
        std::cout << "   Game has not written any graphics to screen yet!" << std::endl;
    } else {
        std::cout << "✅ Nametable has content!" << std::endl;
    }

    return 0;
}
