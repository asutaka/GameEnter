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

    std::cout << "=== Extended Framebuffer Test ===" << std::endl;
    emu.reset();
    
    // Run many frames to wait for game to initialize
    std::cout << "Running 300 frames (5 seconds)..." << std::endl;
    for (int i = 0; i < 300; i++) {
        emu.run_frame();
        
        if (i % 60 == 0) {
            std::cout << "Frame " << i << "..." << std::endl;
        }
    }
    
    // Get framebuffer pointer
    const uint8_t* fb = emu.get_framebuffer();
    
    // Check colors
    std::map<uint32_t, int> color_histogram;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        color_histogram[color]++;
    }
    
    std::cout << "\n=== Framebuffer Analysis ===" << std::endl;
    std::cout << "Unique colors: " << color_histogram.size() << std::endl;
    
    if (color_histogram.size() <= 10) {
        std::cout << "\nColor histogram:" << std::endl;
        for (const auto& pair : color_histogram) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') 
                     << pair.first << ": " << std::dec << pair.second << " pixels" << std::endl;
        }
    }
    
    if (color_histogram.size() > 1) {
        std::cout << "\n✓ SUCCESS: Framebuffer has variation - rendering is working!" << std::endl;
    } else {
        std::cout << "\n✗ FAILED: Framebuffer is still uniform" << std::endl;
    }
    
    return 0;
}
