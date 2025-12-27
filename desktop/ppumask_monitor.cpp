#include "../core/emulator.h"
#include <iostream>
#include <iomanip>

using namespace nes;

// Hook vào PPU để log PPUMASK writes
class PPUMonitor {
public:
    static void monitor_ppumask_writes(Emulator& emu, int frames) {
        for (int frame = 0; frame < frames; frame++) {
            emu.run_frame();
            
            if (frame < 10 || frame % 60 == 0) {
                std::cout << "Frame " << std::dec << frame << ": ";
                std::cout << "Scanline=" << emu.ppu_.get_scanline() 
                         << ", Cycle=" << emu.ppu_.get_cycle() << std::endl;
            }
        }
    }
};

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

    std::cout << "=== PPUMASK Monitor Test ===" << std::endl;
    emu.reset();
    
    // Simulate pressing START button
    std::cout << "Simulating START button press..." << std::endl;
    emu.set_controller(0, 0x08); // START button
    
    // Run frames
    std::cout << "Running 300 frames..." << std::endl;
    PPUMonitor::monitor_ppumask_writes(emu, 300);
    
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
        std::cout << "✓ SUCCESS: Rendering is working!" << std::endl;
    } else {
        std::cout << "✗ FAILED: Still no rendering" << std::endl;
    }
    
    return 0;
}
