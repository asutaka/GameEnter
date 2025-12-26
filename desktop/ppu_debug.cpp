#include "../core/emulator.h"
#include <iostream>
#include <iomanip>

using namespace nes;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file.nes>" << std::endl;
        return 1;
    }
    
    Emulator emu;
    
    std::cout << "Loading ROM: " << argv[1] << std::endl;
    if (!emu.load_rom(argv[1])) {
        std::cerr << "Failed to load ROM!" << std::endl;
        return 1;
    }
    
    emu.reset();
    
    std::cout << "\n=== Running 120 frames and checking PPU state ===\n" << std::endl;
    
    // Run frames and check state periodically
    for (int frame = 0; frame < 120; frame++) {
        emu.run_frame();
        
        if (frame % 30 == 0) {
            std::cout << "\n--- Frame " << frame << " ---" << std::endl;
            
            // Check framebuffer Sample pixels
            const uint8_t* fb = emu.get_framebuffer();
            
            std::cout << "Framebuffer samples:" << std::endl;
            std::cout << "  Pixel (0,0): RGB(" << (int)fb[0] << ", " << (int)fb[1] << ", " << (int)fb[2] << ")" << std::endl;
            std::cout << "  Pixel (128,120): RGB(" << (int)fb[(120*256+128)*4] << ", " << (int)fb[(120*256+128)*4+1] << ", " << (int)fb[(120*256+128)*4+2] << ")" << std::endl;
            
            // Check if all pixels are the same
            bool all_same = true;
            uint8_t first_r = fb[0], first_g = fb[1], first_b = fb[2];
            for (int i = 0; i < 256 * 240; i++) {
                if (fb[i*4] != first_r || fb[i*4+1] != first_g || fb[i*4+2] != first_b) {
                    all_same = false;
                    break;
                }
            }
            
            if (all_same) {
                std::cout << "  ⚠️  All pixels are same color: RGB(" << (int)first_r << ", " << (int)first_g << ", " << (int)first_b << ")" << std::endl;
            } else {
                std::cout << "  ✅ Multiple colors detected!" << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    
    return 0;
}
