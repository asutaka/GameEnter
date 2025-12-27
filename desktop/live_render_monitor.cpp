#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <chrono>
#include <thread>

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

    std::cout << "=== Live Rendering Monitor ===" << std::endl;
    emu.reset();
    
    // Simulate START button after 3 seconds
    for (int frame = 0; frame < 600; frame++) {
        // Auto-press START at frame 180-190
        if (frame >= 180 && frame <= 190) {
            emu.set_controller(0, 0x08); // START
        } else {
            emu.set_controller(0, 0x00);
        }
        
        emu.run_frame();
        
        // Check every 60 frames
        if (frame > 0 && frame % 60 == 0) {
            const uint8_t* fb = emu.get_framebuffer();
            std::map<uint32_t, int> colors;
            
            for (int i = 0; i < 256*240; i++) {
                uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
                colors[color]++;
            }
            
            std::cout << "Frame " << std::setw(3) << frame << ": " 
                     << colors.size() << " colors";
            
            if (colors.size() > 1) {
                std::cout << " ✓ GRAPHICS!";
                // Show top 3 colors
                int count = 0;
                for (auto it = colors.rbegin(); it != colors.rend() && count < 3; ++it, ++count) {
                    std::cout << " #" << std::hex << std::setw(6) << std::setfill('0') << it->first;
                }
                std::cout << std::dec;
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\nTest complete!" << std::endl;
    return 0;
}
