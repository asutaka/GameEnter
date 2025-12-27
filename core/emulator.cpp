#include "emulator.h"
#include <cstring>
#include <cstdio> // For printf

namespace nes {

Emulator::Emulator() : master_clock_(0), audio_time_(0.0) {
    memset(framebuffer_, 0, sizeof(framebuffer_));
    
    // Kết nối các component
    cpu_.connect_memory(&memory_);
    memory_.connect_ppu(&ppu_);
    memory_.connect_apu(&apu_);
    memory_.connect_input(&input_);
    memory_.connect_cartridge(&cartridge_);
    
    // PPU cần access cartridge để đọc CHR ROM (pattern tables)
    ppu_.connect_cartridge(&cartridge_);
    
    // APU cần access memory cho DMC
    apu_.connect_memory(&memory_);
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
    input_.reset();
    memory_.reset();
    cartridge_.reset();
    master_clock_ = 0;
    audio_time_ = 0.0;
    audio_samples_.clear();
}

void Emulator::run_frame() {
    // NES chạy @ 60 FPS (NTSC)
    // 1 frame = 29780.5 CPU cycles
    // CPU:PPU ratio = 1:3
    
    const int CYCLES_PER_FRAME = 29781;
    int cycles = 0;
    
    // Audio settings
    const double CPU_FREQ = 1789773.0;
    const double SAMPLE_RATE = 44100.0;
    const double CYCLES_PER_SAMPLE = CPU_FREQ / SAMPLE_RATE;
    
    audio_samples_.clear();
    
    int instruction_count = 0;
    
    // DEBUG: Log first few frames
    // if (master_clock_ == 0) {
    //     printf("[EMU] Frame 0 starting, PC=$%04X\n", cpu_.PC);
    //     fflush(stdout);
    // }
    
    while (cycles < CYCLES_PER_FRAME) {
        // CPU step
        int cpu_cycles = cpu_.step();
        
        // DEBUG: Check if CPU is actually executing
        // if (master_clock_ == 0 && instruction_count < 5) {
        //     printf("[EMU] Instruction %d: PC=$%04X, cycles=%d\n", 
        //            instruction_count,cpu_.PC, cpu_cycles);
        //     fflush(stdout);
        // }
        instruction_count++;
        
        cycles += cpu_cycles;
        
        // PPU step (3x faster than CPU)
        for (int i = 0; i < cpu_cycles * 3; i++) {
            ppu_.step();
            if (ppu_.nmi_occurred()) {
                ppu_.clear_nmi();
                cpu_.nmi();
            }
        }
        
        // APU step & Audio Sampling
        for (int i = 0; i < cpu_cycles; i++) {
            apu_.step();
            
            audio_time_ += 1.0;
            if (audio_time_ >= CYCLES_PER_SAMPLE) {
                audio_time_ -= CYCLES_PER_SAMPLE;
                audio_samples_.push_back(apu_.get_sample());
            }
        }
    }
    
    master_clock_ += CYCLES_PER_FRAME;
}

const uint8_t* Emulator::get_framebuffer() const {
    // Return PPU's framebuffer, not the local empty one
    return ppu_.get_framebuffer();
}

void Emulator::set_controller(int controller, uint8_t buttons) {
    // buttons: A, B, Select, Start, Up, Down, Left, Right (bits 0-7)
    for (int i = 0; i < 8; i++) {
        bool pressed = (buttons & (1 << i)) != 0;
        if (controller == 0) {
            input_.set_button_state(i, pressed);
        } else if (controller == 1) {
            // TODO: Controller 2 support if needed
        }
    }
}

const std::vector<float>& Emulator::get_audio_samples() const {
    return audio_samples_;
}

} // namespace nes
