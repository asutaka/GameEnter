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

    std::cout << "=== Deep PPU Debug ===" << std::endl;
    emu.reset();
    
    // Warmup
    for (int i = 0; i < 10; i++) {
        emu.run_frame();
    }
    
    // Initialize v register
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x20);
    emu.memory_.write(0x2006, 0x00);
    
    // Write palette manually
    emu.memory_.read(0x2002);
    emu.memory_.write(0x2006, 0x3F);
    emu.memory_.write(0x2006, 0x00);
    emu.memory_.write(0x2007, 0x0F); // BG = black
    emu.memory_.write(0x2007, 0x30); // Color 1 = white
    emu.memory_.write(0x2007, 0x16); // Color 2 = red  
    emu.memory_.write(0x2007, 0x27); // Color 3 = orange
    
    // Force PPUCTRL and PPUMASK
    emu.memory_.write(0x2000, 0x80); // PPUCTRL
    emu.memory_.write(0x2001, 0x1E); // PPUMASK
    
    // Run 1 frame
    std::cout << "Running 1 frame with rendering..." << std::endl;
    emu.run_frame();
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    
    // Sample some pixels
    std::cout << "\nSampling pixels:" << std::endl;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int idx = (y * 256 + x) * 4;
            uint32_t color = (fb[idx] << 16) | (fb[idx+1] << 8) | fb[idx+2];
            std::cout << std::hex << std::setw(6) << std::setfill('0') << color << " ";
        }
        std::cout << std::endl;
    }
    
    // Count unique colors
    std::map<uint32_t, int> colors;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "\nUnique colors: " << colors.size() << std::endl;
    for (const auto& pair : colors) {
        std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << pair.first 
                 << ": " << std::dec << pair.second << " pixels" << std::endl;
    }
    
    if (colors.size() > 1) {
        std::cout << "\n✓ SUCCESS!" << std::endl;
    } else {
        std::cout << "\n✗ FAILED - uniform color" << std::endl;
        std::cout << "\nThis means PPU rendering has a fundamental bug." << std::endl;
        std::cout << "The issue is likely in render_pixel() or fetch_background_tile()." << std::endl;
    }
    
    return 0;
}
