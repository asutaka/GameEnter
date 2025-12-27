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

    std::cout << "=== Extended Render Test ===" << std::endl;
    emu.reset();
    
    // Try pressing START button to start game
    std::cout << "Pressing START button..." << std::endl;
    emu.set_controller(0, 0x08); // START = bit 3
    
    // Run many frames
    std::cout << "Running 600 frames (10 seconds)..." << std::endl;
    for (int i = 0; i < 600; i++) {
        emu.run_frame();
        
        if (i % 60 == 0) {
            std::cout << "Frame " << i << "..." << std::endl;
        }
        
        // Release START after 10 frames
        if (i == 10) {
            emu.set_controller(0, 0x00);
        }
        
        // Check rendering every 60 frames
        if (i > 0 && i % 60 == 0) {
            const uint8_t* fb = emu.get_framebuffer();
            std::map<uint32_t, int> colors;
            for (int j = 0; j < 256*240; j++) {
                uint32_t color = (fb[j*4] << 16) | (fb[j*4+1] << 8) | fb[j*4+2];
                colors[color]++;
            }
            
            if (colors.size() > 1) {
                std::cout << "  ✓ Frame " << i << ": " << colors.size() << " colors detected!" << std::endl;
            }
        }
    }
    
    // Final check
    const uint8_t* fb = emu.get_framebuffer();
    std::map<uint32_t, int> color_counts;
    
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        color_counts[color]++;
    }
    
    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Unique colors: " << color_counts.size() << std::endl;
    
    if (color_counts.size() > 1) {
        std::cout << "\n✓✓✓ RENDERING IS WORKING! ✓✓✓" << std::endl;
        std::cout << "\nTop colors:" << std::endl;
        int count = 0;
        for (auto it = color_counts.rbegin(); it != color_counts.rend() && count < 10; ++it, ++count) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << it->first 
                     << ": " << std::dec << it->second << " pixels" << std::endl;
        }
    } else {
        std::cout << "\n✗ Still no rendering" << std::endl;
        std::cout << "Single color: #" << std::hex << std::setw(6) << std::setfill('0') 
                 << color_counts.begin()->first << std::dec << std::endl;
    }
    
    return 0;
}
