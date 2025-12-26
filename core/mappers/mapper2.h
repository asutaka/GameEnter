#ifndef NES_MAPPER2_H
#define NES_MAPPER2_H

#include "mappers/mapper.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 2 (UxROM) - Simple PRG banking
 * 
 * Games: Mega Man 1, Castlevania, Contra Force, Duck Tales, etc.
 * 
 * Features:
 * - Switchable 16KB PRG bank at $8000
 * - Fixed 16KB PRG bank at $C000 (last bank)
 * - No CHR banking (8KB CHR RAM)
 */
class Mapper2 : public Mapper {
public:
    Mapper2(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper2() override = default;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;

private:
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    size_t prg_size_;
    size_t chr_size_;
    
    uint8_t prg_bank_;  // Selected 16KB bank at $8000
    uint8_t chr_ram_[0x2000];  // 8KB CHR RAM
};

} // namespace nes

#endif // NES_MAPPER2_H
