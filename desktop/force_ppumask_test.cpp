#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <map>

using namespace nes;

// We need to add a way to read PPU registers
// For now, let's create a simple test that forces PPUMASK after game init

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

    std::cout << "=== Force Rendering After Init ===" << std::endl;
    emu.reset();
    
    // Run 300 frames with START button
    std::cout << "Running 300 frames with START button..." << std::endl;
    for (int frame = 0; frame < 300; frame++) {
        if (frame >= 180 && frame <= 190) {
            emu.set_controller(0, 0x08); // START
        } else {
            emu.set_controller(0, 0x00);
        }
        emu.run_frame();
    }
    
    std::cout << "Trying PPUCTRL = $80 (BG pattern $0000) and PPUMASK = $1E..." << std::endl;
    // Force enable rendering with pattern table $0000
    emu.memory_.write(0x2000, 0x80); // PPUCTRL: NMI enable, BG pattern $0000
    emu.memory_.write(0x2001, 0x1E); // PPUMASK: Show BG + Sprites
    
    // Run more frames
    std::cout << "Running 60 frames..." << std::endl;
    for (int i = 0; i < 60; i++) {
        emu.run_frame();
    }
    
    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    std::map<uint32_t, int> colors;
    
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        colors[color]++;
    }
    
    std::cout << "Result with pattern $0000: " << colors.size() << " colors" << std::endl;
    
    if (colors.size() <= 1) {
        std::cout << "\nTrying PPUCTRL = $90 (BG pattern $1000)..." << std::endl;
        emu.memory_.write(0x2000, 0x90); // PPUCTRL: NMI enable, BG pattern $1000
        
        for (int i = 0; i < 60; i++) {
            emu.run_frame();
        }
        
        colors.clear();
        fb = emu.get_framebuffer();
        for (int i = 0; i < 256*240; i++) {
            uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
            colors[color]++;
        }
        
        std::cout << "Result with pattern $1000: " << colors.size() << " colors" << std::endl;
    }
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Unique colors: " << colors.size() << std::endl;
    
    if (colors.size() > 1) {
        std::cout << "\n✓✓✓ SUCCESS! Graphics appeared after forcing PPUMASK!" << std::endl;
        std::cout << "\nTop colors:" << std::endl;
        int count = 0;
        for (auto it = colors.rbegin(); it != colors.rend() && count < 5; ++it, ++count) {
            std::cout << "  #" << std::hex << std::setw(6) << std::setfill('0') << it->first 
                     << ": " << std::dec << it->second << " pixels" << std::endl;
        }
    } else {
        std::cout << "\n✗ Still no graphics even after forcing PPUMASK" << std::endl;
        std::cout << "Single color: #" << std::hex << std::setw(6) << std::setfill('0') 
                 << colors.begin()->first << std::dec << std::endl;
    }
    
    return 0;
}
