#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <exception>

using namespace nes;

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
            return 1;
        }

        std::cout << "=== CPU Execution Test ===" << std::endl;
        std::cout.flush();
        
        Emulator emu;
        std::cout << "Emulator created." << std::endl;
        std::cout.flush();
        
        if (!emu.load_rom(argv[1])) {
            std::cerr << "Failed to load ROM" << std::endl;
            return 1;
        }

        std::cout << "ROM loaded successfully!" << std::endl;
        std::cout.flush();
        
        emu.reset();
        std::cout << "Reset complete." << std::endl;
        std::cout.flush();
        
        std::cout << "About to access PC..." << std::endl;
        std::cout.flush();
        
        uint16_t pc = emu.cpu_.PC;
        
        std::cout << "PC read OK, value=$" << std::hex << std::setw(4) << std::setfill('0') << pc << std::endl;
        std::cout.flush();
        
        std::cout << "Initial PC: $" << std::hex << std::setw(4) << std::setfill('0') << emu.cpu_.PC << std::endl;
        std::cout << std::endl;
std::cout.flush();
        
        std::cout << "Running 180 frames (3 seconds)..." << std::endl;
        std::cout.flush();
        
        for (int frame = 0; frame < 180; frame++) {
            if (frame % 30 == 0) {
                std::cout << "Frame " << frame << "..." << std::flush;
            }
            emu.run_frame();
            if (frame % 30 == 0) {
                std::cout << " done. PC=$" << std::hex << std::setw(4) << std::setfill('0') << emu.cpu_.PC << std::dec << std::endl;
                std::cout.flush();
            }
        }
        
        std::cout << std::endl;
        std::cout << "Checking framebuffer..." << std::endl;
        std::cout.flush();
        
        const uint8_t* fb = emu.get_framebuffer();
        uint32_t first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
        
        int different = 0;
        for (int i = 0; i < 256*240; i++) {
            uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
            if (color != first_color) different++;
        }
        
        std::cout << "Background color: #" << std::hex << std::setw(6) << std::setfill('0') << first_color << std::dec << std::endl;
        std::cout << "Different pixels: " << different << std::endl;
        std::cout.flush();
        
        if (different == 0) {
            std::cout << std::endl;
            std::cout << "❌ NO RENDERING!" << std::endl;
            std::cout << std::endl;
            std::cout << "Check console output above for [EMU], [MEM], and [PPU] messages." << std::endl;
            std::cout << "If you see NO debug messages, CPU may not be executing." << std::endl;
        } else {
            std::cout << std::endl;
            std::cout << "✓ Rendering working!" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION!" << std::endl;
        return 1;
    }
    
    return 0;
}
