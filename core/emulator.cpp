#include "emulator.h"
#include <cstring>

namespace nes {

Emulator::Emulator() : master_clock_(0) {
    memset(framebuffer_, 0, sizeof(framebuffer_));
    
    // Kết nối các component
    cpu_.connect_memory(&memory_);
    memory_.connect_ppu(&ppu_);
    memory_.connect_apu(&apu_);
    memory_.connect_cartridge(&cartridge_);
    
    // PPU cần access cartridge để đọc CHR ROM (pattern tables)
    ppu_.connect_cartridge(&cartridge_);
}

Emulator::~Emulator() {
}

bool Emulator::load_rom(const std::string& filename) {
    return cartridge_.load_from_file(filename);
}

void Emulator::reset() {
    cpu_.reset();
    ppu_.reset();
    apu_.reset();
    memory_.reset();
    cartridge_.reset();
    master_clock_ = 0;
}

void Emulator::run_frame() {
    // NES chạy @ 60 FPS (NTSC)
    // 1 frame = 29780.5 CPU cycles
    // CPU:PPU ratio = 1:3
    
    const int CYCLES_PER_FRAME = 29781;
    int cycles = 0;
    
    while (cycles < CYCLES_PER_FRAME) {
        // CPU step
        int cpu_cycles = cpu_.step();
        cycles += cpu_cycles;
        
        // PPU step (3x faster than CPU)
        for (int i = 0; i < cpu_cycles * 3; i++) {
            ppu_.step();
        }
        
        // APU step
        apu_.step();
    }
}

const uint8_t* Emulator::get_framebuffer() const {
    // Return PPU's framebuffer, not the local empty one
    return ppu_.get_framebuffer();
}

void Emulator::set_controller(int controller, uint8_t buttons) {
    // TODO: Implement controller input
}

} // namespace nes
