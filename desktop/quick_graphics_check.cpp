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

    std::cout << "=== Quick Graphics Check ===" << std::endl;
    emu.reset();
    
    // Run 120 frames (2 seconds)
    std::cout << "Running 120 frames..." << std::endl;
    for (int i = 0; i < 120; i++) {
        emu.run_frame();
        
        // Check every 30 frames
        if (i > 0 && i % 30 == 0) {
            const uint8_t* fb = emu.get_framebuffer();
            std::map<uint32_t, int> colors;
            
            for (int j = 0; j < 256*240; j++) {
                uint32_t color = (fb[j*4] << 16) | (fb[j*4+1] << 8) | fb[j*4+2];
                colors[color]++;
            }
            
            std::cout << "Frame " << i << ": " << colors.size() << " unique colors" << std::endl;
            
            if (colors.size() > 1) {
                std::cout << "  Colors:" << std::endl;
                int count = 0;
                for (const auto& pair : colors) {
                    if (count++ < 5) {
                        std::cout << "    #" << std::hex << std::setw(6) << std::setfill('0') 
                                 << pair.first << ": " << std::dec << pair.second << " pixels" << std::endl;
                    }
                }
            }
        }
    }
    
    return 0;
}
