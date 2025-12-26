#ifndef NES_MAPPER1_H
#define NES_MAPPER1_H

#include "mappers/mapper.h"
#include "cartridge/cartridge.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 1 (MMC1) - Nintendo's most popular mapper
 * 
 * Games: Zelda, Metroid, Mega Man 2, Final Fantasy, Kid Icarus, etc.
 * 
 * Features:
 * - PRG ROM banking (16KB or 32KB switchable)
 * - CHR ROM banking (4KB or 8KB switchable)
 * - Mirroring control
 * - PRG RAM enable
 */
class Mapper1 : public Mapper {
public:
    Mapper1(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper1() override = default;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;
    
    MirrorMode get_mirroring() const { return mirror_mode_; }

private:
    // ROM pointers
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    size_t prg_size_;
    size_t chr_size_;
    
    // MMC1 registers
    uint8_t shift_register_;   // 5-bit shift register
    uint8_t shift_count_;      // Write counter
    
    // Control register ($8000-$9FFF)
    struct {
        uint8_t mirroring : 2;      // 0=one-screen-lower, 1=one-screen-upper, 2=vertical, 3=horizontal
        uint8_t prg_mode : 2;       // 0,1=32KB; 2=fix first; 3=fix last
        uint8_t chr_mode : 1;       // 0=8KB; 1=4KB
    } control_;
    
    // Bank registers
    uint8_t chr_bank_0_;       // CHR bank 0 ($A000-$BFFF)
    uint8_t chr_bank_1_;       // CHR bank 1 ($C000-$DFFF)
    uint8_t prg_bank_;         // PRG bank ($E000-$FFFF)
    
    // PRG RAM
    uint8_t prg_ram_[0x2000];  // 8KB PRG RAM
    bool prg_ram_enabled_;
    
    // Mirroring
    MirrorMode mirror_mode_;
    
    // Helper functions
    void write_control(uint8_t value);
    void update_mirroring();
    uint32_t get_prg_bank_offset(uint16_t address);
    uint32_t get_chr_bank_offset(uint16_t address);
};

} // namespace nes

#endif // NES_MAPPER1_H
