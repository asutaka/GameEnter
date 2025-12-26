#ifndef NES_EMULATOR_H
#define NES_EMULATOR_H

#include "cpu/cpu.h"
#include "ppu/ppu.h"
#include "apu/apu.h"
#include "input/input.h"
#include "memory/memory.h"
#include "cartridge/cartridge.h"

namespace nes {

/**
 * @brief Class chính của NES Emulator
 * 
 * Quản lý tất cả components và điều phối emulation
 */
class Emulator {
public:
    Emulator();
    ~Emulator();
    
    /**
     * @brief Load ROM file
     */
    bool load_rom(const std::string& filename);
    
    /**
     * @brief Reset emulator
     */
    void reset();
    
    /**
     * @brief Chạy một frame (1/60 giây)
     */
    void run_frame();
    
    /**
     * @brief Lấy framebuffer để render (256x240 pixels, RGBA)
     */
    const uint8_t* get_framebuffer() const;
    
    /**
     * @brief Set controller input
     * @param controller 0 hoặc 1
     * @param buttons Bit mask của buttons (A, B, Select, Start, Up, Down, Left, Right)
     */
    void set_controller(int controller, uint8_t buttons);
    
    /**
     * @brief Get audio samples generated in the last frame
     */
    const std::vector<float>& get_audio_samples() const;

    // Public access cho testing (TODO: Remove sau khi có proper API)
    CPU cpu_;
    Memory memory_;
    Input input_;

private:
    PPU ppu_;
    APU apu_;
    Cartridge cartridge_;
    
    uint8_t framebuffer_[256 * 240 * 4]; // RGBA
    
    // Audio
    std::vector<float> audio_samples_;
    double audio_time_;
    
    // Đồng bộ CPU/PPU timing
    int master_clock_;
};

} // namespace nes

#endif // NES_EMULATOR_H
