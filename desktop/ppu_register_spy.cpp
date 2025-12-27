#include "../core/emulator.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace nes;

// Hook vào PPU để log writes
class PPUSpy {
public:
    static void log_write(uint16_t address, uint8_t value, std::ostream& out) {
        out << "PPU Write: $" << std::hex << std::setw(4) << std::setfill('0') << address;
        out << " = $" << std::hex << std::setw(2) << std::setfill('0') << (int)value;
        
        uint16_t reg = address & 0x0007;
        switch(reg) {
            case 0: out << " (PPUCTRL)"; break;
            case 1: out << " (PPUMASK)"; break;
            case 2: out << " (PPUSTATUS - read only!)"; break;
            case 3: out << " (OAMADDR)"; break;
            case 4: out << " (OAMDATA)"; break;
            case 5: out << " (PPUSCROLL)"; break;
            case 6: out << " (PPUADDR)"; break;
            case 7: out << " (PPUDATA)"; break;
        }
        out << std::endl;
        
        // Special logging for PPUMASK
        if (reg == 1) {
            out << "  -> BG=" << ((value & 0x08) ? "ON" : "OFF");
            out << ", SPR=" << ((value & 0x10) ? "ON" : "OFF");
            out << ", Grayscale=" << ((value & 0x01) ? "ON" : "OFF");
            out << std::endl;
        }
        
        // Special logging for PPUCTRL
        if (reg == 0) {
            out << "  -> NMI=" << ((value & 0x80) ? "ON" : "OFF");
            out << ", SPR_Size=" << ((value & 0x20) ? "8x16" : "8x8");
            out << ", BG_Table=$" << ((value & 0x10) ? "1000" : "0000");
            out << std::endl;
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

    std::cout << "=== PPU Register Spy Tool ===" << std::endl;
    std::cout << "ROM loaded successfully!" << std::endl;
    std::cout << "Running for 60 frames and logging PPU writes..." << std::endl;
    std::cout << std::endl;

    emu.reset();

    // Create log file
    std::ofstream log("ppu_writes.log");
    
    int ppu_writes = 0;
    int ppumask_writes = 0;
    uint8_t last_ppumask = 0;
    
    // Intercept memory writes to PPU
    // We'll run and then check memory writes
    for (int frame = 0; frame < 60; frame++) {
        emu.run_frame();
        
        if (frame % 10 == 0) {
            std::cout << "Frame " << frame << "..." << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "=== Analysis ===" << std::endl;
    std::cout << "Note: Cannot directly spy on writes without modifying core." << std::endl;
    std::cout << "Checking framebuffer instead..." << std::endl;
    std::cout << std::endl;

    // Check framebuffer
    const uint8_t* fb = emu.get_framebuffer();
    uint32_t first_color = (fb[0] << 16) | (fb[1] << 8) | fb[2];
    
    int different_pixels = 0;
    for (int i = 0; i < 256*240; i++) {
        uint32_t color = (fb[i*4] << 16) | (fb[i*4+1] << 8) | fb[i*4+2];
        if (color != first_color) different_pixels++;
    }

    std::cout << "Background color: #" << std::hex << std::setw(6) << std::setfill('0') << first_color << std::dec << std::endl;
    std::cout << "Different pixels: " << different_pixels << std::endl;
    std::cout << std::endl;

    if (different_pixels == 0) {
        std::cout << "❌ PROBLEM: NO RENDERING AT ALL" << std::endl;
        std::cout << std::endl;
        std::cout << "This means either:" << std::endl;
        std::cout << "  1. PPU PPUMASK was never enabled (show_bg=0, show_sprites=0)" << std::endl;
        std::cout << "  2. PPU rendering logic has a critical bug" << std::endl;
        std::cout << "  3. CPU is not executing game code properly" << std::endl;
        std::cout << std::endl;
        std::cout << "NEXT STEPS:" << std::endl;
        std::cout << "  → Add printf debugging to PPU::write_register()" << std::endl;
        std::cout << "  → Add printf debugging to PPU::render_pixel()" << std::endl;
        std::cout << "  → Check if CPU is executing instructions" << std::endl;
    } else {
        std::cout << "✓ Rendering is working!" << std::endl;
    }

    return 0;
}
