#ifndef NES_MAPPER7_H
#define NES_MAPPER7_H

#include "mappers/mapper.h"
#include "cartridge/cartridge.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 7 (AxROM) - 32KB PRG banking with one-screen mirroring
 * 
 * Games: Battletoads, Wizards & Warriors, Marble Madness, etc.
 * 
 * Features:
 * - Switchable 32KB PRG bank
 * - One-screen mirroring control
 * - CHR RAM (8KB)
 */
class Mapper7 : public Mapper {
public:
    Mapper7(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper7() override = default;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;
    
    MirrorMode get_mirroring() const override { return mirror_mode_; }

private:
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    size_t prg_size_;
    size_t chr_size_;
    
    uint8_t prg_bank_;      // Selected 32KB bank
    MirrorMode mirror_mode_; // Single-screen mirroring
    uint8_t chr_ram_[0x2000];  // 8KB CHR RAM
};

} // namespace nes

#endif // NES_MAPPER7_H
