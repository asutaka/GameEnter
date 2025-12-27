#include "../core/emulator.h"
#include "../core/ppu/ppu.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace nes;

// Helper to access PPU internals (we'll need to make some members public temporarily)
class PPUDebugger {
public:
    static void dump_state(PPU& ppu, std::ostream& out) {
        out << "PPU State:" << std::endl;
        out << "  Scanline: " << ppu.get_scanline() << std::endl;
        out << "  Cycle: " << ppu.get_cycle() << std::endl;
    }
    
    static void dump_framebuffer_stats(const uint8_t* fb, std::ostream& out) {
        std::map<uint32_t, int> colors;
        for (int i = 0; i < 256*240; i++) {
            uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
            colors[color]++;
        }
        
        out << "Framebuffer colors: " << colors.size() << std::endl;
        for (const auto& pair : colors) {
            out << "  #" << std::hex << std::setw(6) << std::setfill('0') 
                << pair.first << ": " << std::dec << pair.second << " pixels" << std::endl;
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

    std::cout << "=== Deep PPU Diagnostic ===" << std::endl;
    emu.reset();
    
    std::ofstream log("ppu_deep_debug.log");
    
    // Run 10 frames
    for (int frame = 0; frame < 10; frame++) {
        log << "\n=== Frame " << frame << " ===" << std::endl;
        emu.run_frame();
        
        PPUDebugger::dump_state(emu.ppu_, log);
        
        if (frame == 9) {
            const uint8_t* fb = emu.get_framebuffer();
            PPUDebugger::dump_framebuffer_stats(fb, log);
            PPUDebugger::dump_framebuffer_stats(fb, std::cout);
        }
    }
    
    std::cout << "Debug log written to ppu_deep_debug.log" << std::endl;
    
    return 0;
}
