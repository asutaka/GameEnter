#ifndef NES_MAPPER0_H
#define NES_MAPPER0_H

#include "mappers/mapper.h"
#include <cstdint>

namespace nes {

/**
 * @brief Mapper 0 (NROM) - Simplest mapper, no banking
 * 
 * Games: Donkey Kong, Super Mario Bros, Ice Climber, Excitebike, Pac-Man, etc.
 * 
 * Features:
 * - No PRG ROM banking (16KB or 32KB fixed)
 * - No CHR ROM banking (8KB fixed)
 * - Optional 8KB PRG RAM
 * - Mirroring from iNES header
 * 
 * CPU Memory Map:
 * $6000-$7FFF: PRG RAM (8KB) - Optional
 * $8000-$BFFF: First 16KB of PRG ROM
 * $C000-$FFFF: Last 16KB of PRG ROM (or mirror if only 16KB total)
 * 
 * PPU Memory Map:
 * $0000-$1FFF: CHR ROM (8KB)
 */
class Mapper0 : public Mapper {
public:
    Mapper0(uint8_t* prg_rom, size_t prg_size,
            uint8_t* chr_rom, size_t chr_size);
    ~Mapper0() override;
    
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
    void reset() override;

private:
    uint8_t* prg_rom_;
    uint8_t* chr_rom_;
    uint8_t* prg_ram_;
    size_t prg_size_;
    size_t chr_size_;
};

} // namespace nes

#endif // NES_MAPPER0_H
