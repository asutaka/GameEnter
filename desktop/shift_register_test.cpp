#include "../core/emulator.h"
#include "../core/ppu/ppu.h"
#include <iostream>
#include <iomanip>
#include <fstream>

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

    std::cout << "=== Shift Register Diagnostic ===" << std::endl;
    emu.reset();
    
    // Force enable rendering
    std::cout << "Forcing PPUMASK = $1E..." << std::endl;
    emu.memory_.write(0x2001, 0x1E);
    
    // Run a few frames
    std::cout << "Running 10 frames..." << std::endl;
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    
    std::cout << "\n=== Framebuffer Sample ===" << std::endl;
    std::cout << "First 10 pixels:" << std::endl;
    for (int i = 0; i < 10; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        std::cout << "  Pixel " << i << ": #" << std::hex << std::setw(6) << std::setfill('0') 
                 << color << std::dec << std::endl;
    }
    
    // Sample middle of screen
    std::cout << "\nMiddle scanline (120), pixels 120-130:" << std::endl;
    for (int x = 120; x < 130; x++) {
        int idx = (120 * 256 + x) * 4;
        uint32_t color = (fb[idx] << 16) | (fb[idx+1] << 8) | fb[idx+2];
        std::cout << "  Pixel " << x << ": #" << std::hex << std::setw(6) << std::setfill('0') 
                 << color << std::dec << std::endl;
    }
    
    return 0;
}
